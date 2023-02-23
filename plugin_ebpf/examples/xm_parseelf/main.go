/*
 * @Author: CALM.WU
 * @Date: 2023-02-23 14:32:50
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-23 17:28:33
 */

package main

import (
	"debug/elf"
	"flag"
	"fmt"
	"os"
)

var (
	__elfFile string
	__fs      = flag.NewFlagSet("parse.elf", flag.ContinueOnError)
)

func init() {
	__fs.StringVar(&__elfFile, "elf", "../plugin_ebpf/bpf/.output/xm_cachestat.bpf.o", "elf file path")
	__fs.SetOutput(os.Stdout)
}

func main() {
	if err := __fs.Parse(os.Args[1:]); err != nil {
		fmt.Fprintln(os.Stderr, "Error:", err)
		return
	}

	file, err := elf.Open(__elfFile)
	if err != nil {
		fmt.Fprintln(os.Stderr, "read ELF file failed. err:", err)
		return
	}
	defer file.Close()

	// section
	for i, sec := range file.Sections {
		idx := elf.SectionIndex(i)
		fmt.Printf("section idx:%d, idxName:'%s', name:'%s', type:'%s'\n", idx, idx.String(), sec.Name, sec.Type.String())
	}

	if symbols, err := file.Symbols(); err != nil {
		fmt.Fprintln(os.Stderr, "load symbols failed. err:", err)
	} else {
		for i, sym := range symbols {
			symType := elf.ST_TYPE(sym.Info)
			fmt.Printf("symbol[%d] name:'%s', type:'%s', section:'%s[%s]', value:'%d', size:'%d'\n",
				i, sym.Name, symType.String(), file.Sections[sym.Section].Name, sym.Section.String(), sym.Value, sym.Size)
		}
	}
}

/*
symbol[0] name:'', type:'STT_SECTION', section:'SHN_UNDEF+3', value:'0', size:'0'
symbol[1] name:'LBB0_2', type:'STT_NOTYPE', section:'SHN_UNDEF+3', value:'256', size:'0'
symbol[2] name:'LBB0_3', type:'STT_NOTYPE', section:'SHN_UNDEF+3', value:'272', size:'0'
symbol[3] name:'', type:'STT_SECTION', section:'SHN_UNDEF+5', value:'0', size:'0'
symbol[4] name:'LBB1_2', type:'STT_NOTYPE', section:'SHN_UNDEF+5', value:'192', size:'0'
symbol[5] name:'LBB1_3', type:'STT_NOTYPE', section:'SHN_UNDEF+5', value:'208', size:'0'
symbol[6] name:'', type:'STT_SECTION', section:'SHN_UNDEF+7', value:'0', size:'0'
symbol[7] name:'LBB2_2', type:'STT_NOTYPE', section:'SHN_UNDEF+7', value:'192', size:'0'
symbol[8] name:'LBB2_3', type:'STT_NOTYPE', section:'SHN_UNDEF+7', value:'208', size:'0'
symbol[9] name:'', type:'STT_SECTION', section:'SHN_UNDEF+9', value:'0', size:'0'
symbol[10] name:'LBB3_2', type:'STT_NOTYPE', section:'SHN_UNDEF+9', value:'192', size:'0'
symbol[11] name:'LBB3_3', type:'STT_NOTYPE', section:'SHN_UNDEF+9', value:'208', size:'0'
symbol[12] name:'xm_kp_cs_atpcl', type:'STT_FUNC', section:'SHN_UNDEF+3', value:'0', size:'288'
symbol[13] name:'xm_page_cache_ops_count', type:'STT_OBJECT', section:'SHN_UNDEF+13', value:'0', size:'32'
symbol[14] name:'xm_kp_cs_mpa', type:'STT_FUNC', section:'SHN_UNDEF+5', value:'0', size:'224'
symbol[15] name:'xm_kp_cs_apd', type:'STT_FUNC', section:'SHN_UNDEF+7', value:'0', size:'224'
symbol[16] name:'xm_kp_cs_mbd', type:'STT_FUNC', section:'SHN_UNDEF+9', value:'0', size:'224'
symbol[17] name:'xm_cs_total', type:'STT_OBJECT', section:'SHN_UNDEF+11', value:'0', size:'8'
symbol[18] name:'xm_cs_misses', type:'STT_OBJECT', section:'SHN_UNDEF+11', value:'8', size:'8'
symbol[19] name:'xm_cs_mbd', type:'STT_OBJECT', section:'SHN_UNDEF+11', value:'16', size:'8'
symbol[20] name:'_license', type:'STT_OBJECT', section:'SHN_UNDEF+12', value:'0', size:'4'
*/
