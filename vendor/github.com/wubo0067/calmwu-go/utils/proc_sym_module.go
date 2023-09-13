/*
 * @Author: CALM.WU
 * @Date: 2023-01-10 14:20:15
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-09-12 18:33:05
 */

package utils

import (
	"bufio"
	"debug/elf"
	"fmt"
	"os"
	"sort"
	"strings"
	"syscall"

	"github.com/pkg/errors"
	"golang.org/x/sys/unix"
)

const (
	__miniProcMapsEntryDefaultFieldCount = 6
)

type ProcSymModuleType int

var (
	ErrProcModuleNotSupport       = errors.New("proc module not support")
	ErrProcModuleNotSymbolSection = errors.New("proc module not symbol section")
)

const (
	UNKNOWN ProcSymModuleType = iota
	EXEC
	SO
	VDSO
)

type ProcSym struct {
	pc   uint64
	name string
}

type ProcMapPermissions struct {
	// Readable is true if the mapping is readable.
	Readable bool
	// Writable is true if the mapping is writable.
	Writable bool
	// Executable is true if the mapping is executable.
	Executable bool
	// Shared is true if the mapping is shared.
	Shared bool
	// Private is true if the mapping is private.
	Private bool
}

type ProcSymsModule struct {
	// StartAddr is the starting pc of current mapping.
	StartAddr uint64
	// EndAddr is the ending pc of current mapping.
	EndAddr uint64
	// Perm is the permission of current mapping.
	Perms ProcMapPermissions
	// Offset is the offset of current mapping.
	Offset uint64
	// Dev is the device of current mapping.
	Dev uint64
	// Inode is the inode of current mapping. find / -inum 101417806 or lsof -n -i 'inode=174919'
	Inode uint64
	//
	Pathname string
	//
	Type ProcSymModuleType
	//
	procSymTable []*ProcSym
	//
	goSymTable *GoSymTable
}

func (psm *ProcSymsModule) open(pid int) (*os.File, *elf.File, error) {
	// rootfs: /proc/%d/root
	var (
		f    *os.File
		elfF *elf.File
		err  error
	)
	modulePath := fmt.Sprintf("/proc/%d/root%s", pid, psm.Pathname)
	f, err = os.OpenFile(modulePath, os.O_RDONLY, 0)
	if err != nil {
		return nil, nil, errors.Wrapf(err, "open module file:'%s'.", modulePath)
	}

	elfF, err = elf.NewFile(f)
	if err != nil {
		f.Close()
		return nil, nil, errors.Wrap(err, "new ELF file.")
	}

	return f, elfF, nil
}

// It reads the contents of /proc/pid/maps, parses each line, and returns a slice of ProcMap entries.
func (psm *ProcSymsModule) loadProcModule(pid int) error {
	var (
		f    *os.File
		elfF *elf.File
		err  error
	)

	// 打开elf文件
	if f, elfF, err = psm.open(pid); err != nil {
		return errors.Wrapf(err, "psm open:'/proc/%d/root%s'.", pid, psm.Pathname)
	}
	defer f.Close()

	// 获取module类型
	switch elfF.Type {
	case elf.ET_EXEC:
		psm.Type = EXEC
	case elf.ET_DYN:
		psm.Type = SO
	default:
		return ErrProcModuleNotSupport
	}

	// from .text section read symbol and pc
	symbols, err := elfF.Symbols()
	if err != nil && !errors.Is(err, elf.ErrNoSymbols) {
		return errors.Wrapf(err, "read elfFile:'%s' SYMTAB.", psm.Pathname)
	}

	dynSymbols, err := elfF.DynamicSymbols()
	if err != nil && !errors.Is(err, elf.ErrNoSymbols) {
		return errors.Wrapf(err, "read elfFile:'%s' DYNSYM.", psm.Pathname)
	}

	total := len(symbols) + len(dynSymbols)
	if total == 0 {
		return ErrProcModuleNotSymbolSection
	}

	for _, symbol := range symbols {
		if int(symbol.Section) < len(elfF.Sections) &&
			elfF.Sections[symbol.Section].Name == ".text" &&
			len(symbol.Name) > 0 {
			ps := new(ProcSym)
			ps.name = symbol.Name
			ps.pc = symbol.Value
			//fmt.Printf("module:'%s' symbol:'%s' pc:'%d'.\n", psm.Pathname, ps.name, ps.pc)
			psm.procSymTable = append(psm.procSymTable, ps)
		}
	}

	// 按地址排序，地址相同按名字排序
	sort.Slice(psm.procSymTable, func(i, j int) bool {
		if psm.procSymTable[i].pc == psm.procSymTable[j].pc {
			return psm.procSymTable[i].name < psm.procSymTable[j].name
		}
		return psm.procSymTable[i].pc < psm.procSymTable[j].pc
	})

	return nil
}

// A method of the ProcSymsModule struct. It is used to print the ProcSymsModule struct.
func (psm *ProcSymsModule) String() string {
	return fmt.Sprintf("%x-%x %#v %x %x %d %s, symbols:%d",
		psm.StartAddr, psm.EndAddr, psm.Perms, psm.Offset, psm.Dev, psm.Inode, psm.Pathname, len(psm.procSymTable))
}

func (psm *ProcSymsModule) __resolvePC(pc uint64) (string, uint32, string, error) {
	// 二分查找
	index := sort.Search(len(psm.procSymTable), func(i int) bool {
		return psm.procSymTable[i].pc > pc
	})

	// addr小于所有symbol的最小地址
	if index == 0 {
		return "", 0, "", errors.Errorf("can't find symbol in module:'%s'", psm.Pathname)
	}

	// 找到了
	ps := psm.procSymTable[index-1]
	return ps.name, uint32(pc - ps.pc), psm.Pathname, nil
}

type ProcSyms struct {
	// pid
	Pid int
	// ProcSymsModule slice
	Modules []*ProcSymsModule
	// inode, Determine whether to refresh
	InodeID uint64
}

// It parses a line from the /proc/<pid>/maps file and returns a ProcSymsModule struct
func __parseProcMapEntry(line string, pss *ProcSyms) error {
	// 7ff8be1a5000-7ff8be1c0000 r-xp 00000000 fd:00 570150                     /usr/lib64/libpthread-2.28.so
	var (
		err                error
		perms              string
		devMajor, devMinor uint64
	)

	fields := strings.Fields(line)
	field_count := len(fields)
	if field_count != __miniProcMapsEntryDefaultFieldCount {
		return nil
	}

	psm := new(ProcSymsModule)
	psm.Type = UNKNOWN

	fmt.Sscanf(line, "%x-%x %s %x %x:%x %d %s", &psm.StartAddr, &psm.EndAddr, &perms,
		&psm.Offset, &devMajor, &devMinor, &psm.Inode, &psm.Pathname)

	//fmt.Printf("parse line:'%s'\n", line)

	if len(psm.Pathname) == 0 ||
		strings.Contains(psm.Pathname, "[vdso]") ||
		strings.Contains(psm.Pathname, "[vsyscall]") {
		return nil
	}

	permBytes := String2Bytes(perms)
	if permBytes[2] != 'x' {
		return nil
	}

	for _, ch := range perms {
		switch ch {
		case 'r':
			psm.Perms.Readable = true
		case 'w':
			psm.Perms.Writable = true
		case 'x':
			psm.Perms.Executable = true
		case 's':
			psm.Perms.Shared = true
		case 'p':
			psm.Perms.Private = true
		}
	}

	psm.Dev = unix.Mkdev(uint32(devMajor), uint32(devMinor))

	// 测试golang程序的load
	if err = psm.loadProcGoModule(pss.Pid); err != nil {
		if err = psm.loadProcModule(pss.Pid); err != nil {
			if errors.Is(err, ErrProcModuleNotSupport) {
				//fmt.Printf("module:'%s' not support read symbols.\n", psm.Pathname)
				return nil
			}
			return errors.Wrapf(err, "load module:'%s' failed.", psm.Pathname)
		}
	}

	pss.Modules = append(pss.Modules, psm)

	return nil
}

// It reads the contents of /proc/pid/maps, parses each line, and returns a slice of ProcMap entries.
func NewProcSyms(pid int) (*ProcSyms, error) {
	procMapsFile, err := os.Open(fmt.Sprintf("/proc/%d/maps", pid))
	if err != nil {
		return nil, errors.Wrap(err, "NewProcMap open failed")
	}
	defer procMapsFile.Close()

	fileExe, err := os.Stat(fmt.Sprintf("/proc/%d/exe", pid))
	if err != nil {
		return nil, errors.Wrap(err, "stat execute file failed.")
	}
	stat := fileExe.Sys().(*syscall.Stat_t)

	pss := new(ProcSyms)
	pss.Pid = pid
	pss.InodeID = stat.Ino

	scanner := bufio.NewScanner(procMapsFile)

	for scanner.Scan() {
		// maps每一行的信息
		text := scanner.Text()
		err := __parseProcMapEntry(text, pss)
		if err != nil {
			return nil, errors.Wrapf(err, "parse text:'%s' failed", text)
		}
	}

	return pss, nil
}

// Used to find the symbol of the specified pc.
func (pss *ProcSyms) ResolvePC(pc uint64) (string, uint32, string, error) {
	if len(pss.Modules) == 0 {
		return "", 0, "", errors.New("proc modules is empty")
	}

	for _, psm := range pss.Modules {
		if pc >= psm.StartAddr && pc <= psm.EndAddr {
			if psm.Type == SO {
				return psm.__resolvePC(pc - psm.StartAddr)
			} else if psm.Type == EXEC {
				//fmt.Printf("module:'%s' pc:'%x' is executable.\n", psm.Pathname, pc)
				if psm.goSymTable != nil {
					symName, offset, err := psm.goSymTable.__resolveGoPC(pc)
					if err == nil {
						return symName, offset, psm.Pathname, nil
					}
				} else {
					return psm.__resolvePC(pc)
				}
			}
		}
	}
	return "", 0, "", errors.Errorf("pc:0x%x resolution failure", pc)
}
