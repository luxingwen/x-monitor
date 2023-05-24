/*
 * @Author: CALM.WU
 * @Date: 2023-02-24 10:56:54
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-24 11:26:12
 */

package collector

import (
	"reflect"
	"strings"

	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/link"
	"github.com/golang/glog"
	"github.com/pkg/errors"
)

// AttachObjPrograms attaches the eBPF programs described by progSpecs to the
// interfaces described by progs.
//
// progs is a map from interface name to interface object. Each interface
// object must be a pointer to a struct or an interface that contains a
// pointer to a struct. The struct must contain a field of type
// netlink.LinkAttrs. This is the same interface that is used by netlink.Link.
// The field must be exported (i.e. start with a capital letter).
//
// progSpecs is a map from program name to program spec. The program name is
// used to lookup the appropriate interface object. For each program name, the
// corresponding interface object is retrieved from progs, and the program is
// attached to that object.
//
// The return value is a slice of link.Link objects. Each object corresponds
// to a single program attached to a single interface. The caller is
// responsible for closing the links.
func AttachObjPrograms(progs interface{}, progSpecs map[string]*ebpf.ProgramSpec) ([]link.Link, error) {
	var links []link.Link

	objProgs := reflect.Indirect(reflect.ValueOf(progs))
	objProgsT := objProgs.Type()

	numFields := objProgsT.NumField()

	for i := 0; i < numFields; i++ {
		objProgsField := objProgsT.Field(i)
		objProgsUnit := objProgs.FieldByName(objProgsField.Name)

		ebpfProgName := objProgsField.Tag.Get("ebpf")
		// ObjProgs:'XMCacheStatPrograms' field name:'XmKpCsApd', kind:'*ebpf.Program', value:'ptr', valueType:'Kprobe(xm_kp_cs_apd)#13'
		// glog.Infof("ObjProgs:'%s' field name:'%s', program:'%s', kind:'%v', value:'%v', valueType:'%v'", objProgsT.Name(),
		// 	objProgsField.Name, ebpfProgName, objProgsField.Type.String(), objProgsField.Type.Kind(), objProgsUnit, objProgsUnit.Type())

		if objProgsField.Type.Kind() == reflect.Ptr {
			bpfProg := objProgsUnit.Interface().(*ebpf.Program)

			// glog.Infof("eBPF program:'%s', type:'%s'", bpfProg.String(), bpfProg.Type().String())
			var progSpec *ebpf.ProgramSpec
			var ok bool
			if progSpec, ok = progSpecs[ebpfProgName]; !ok {
				err := errors.Errorf("ObjProgs:'%s' field name:'%s', program:'%s' not find in ProgramSpec Map",
					objProgsT.Name(), objProgsField.Name, ebpfProgName)
				glog.Error(err.Error())
				return nil, err
			}

			switch bpfProg.Type() {
			case ebpf.Kprobe:
				// todo: 要判断progSpec.AttachTo这个是否在/proc/kallsyms存在，不然就是一个非法的绑定

				linkKP, err := link.Kprobe(progSpec.AttachTo, bpfProg, nil)
				if err == nil {
					links = append(links, linkKP)
					glog.Infof("ObjProgs:'%s' field name:'%s', attach kprobe program:'%s' ===> target:'%s' successed.",
						objProgsT.Name(), objProgsField.Name, ebpfProgName, progSpec.AttachTo)
				} else {
					err := errors.Wrapf(err, "ObjProgs:'%s' field name:'%s', attach kprobe program:'%s' ===> target:'%s' failed.",
						objProgsT.Name(), objProgsField.Name, ebpfProgName, progSpec.AttachTo)
					glog.Error(err.Error())
					return nil, err
				}
			case ebpf.RawTracepoint:
				linkRawTP, err := link.AttachRawTracepoint(link.RawTracepointOptions{
					Name:    progSpec.AttachTo,
					Program: bpfProg,
				})
				if err == nil {
					links = append(links, linkRawTP)
					glog.Infof("ObjProgs:'%s' field name:'%s', attach raw_tracepoint program:'%s' ===> target:'%s' successed.",
						objProgsT.Name(), objProgsField.Name, ebpfProgName, progSpec.AttachTo)
				} else {
					err := errors.Wrapf(err, "ObjProgs:'%s' field name:'%s', attach raw_tracepoint program:'%s' ===> target:'%s' failed.",
						objProgsT.Name(), objProgsField.Name, ebpfProgName, progSpec.AttachTo)
					glog.Error(err.Error())
					return nil, err
				}
			case ebpf.TracePoint:
				// from spec section get group name
				secUnits := strings.Split(progSpec.SectionName, "/")
				group := secUnits[1]
				name := secUnits[2]
				linkTP, err := link.Tracepoint(group, name, bpfProg, nil)
				if err == nil {
					links = append(links, linkTP)
					glog.Infof("ObjProgs:'%s' field name:'%s' group:'%s', attach tracepoint program:'%s' ===> target:'%s' successed.",
						objProgsT.Name(), objProgsField.Name, group, ebpfProgName, name)
				} else {
					err := errors.Wrapf(err, "ObjProgs:'%s' field name:'%s' group:'%s', attach tracepoint program:'%s' ===> target:'%s' failed.",
						objProgsT.Name(), objProgsField.Name, group, ebpfProgName, name)
					glog.Error(err.Error())
					return nil, err
				}
			case ebpf.Tracing:
				linkTracing, err := link.AttachTracing(link.TracingOptions{Program: bpfProg})
				if err == nil {
					links = append(links, linkTracing)
					glog.Infof("ObjProgs:'%s' field name:'%s', attach tracing program:'%s' ===> target:'%s' successed.",
						objProgsT.Name(), objProgsField.Name, ebpfProgName, progSpec.AttachTo)
				} else {
					err := errors.Wrapf(err, "ObjProgs:'%s' field name:'%s', attach tracepoint program:'%s' ===> target:'%s' failed.",
						objProgsT.Name(), objProgsField.Name, ebpfProgName, progSpec.AttachTo)
					glog.Error(err.Error())
					return nil, err
				}
			default:
				err := errors.Errorf("ObjProgs:'%s' field name:'%s', program type:'%s' not support",
					objProgsT.Name(), objProgsField.Name, bpfProg.Type().String())
				glog.Error(err.Error())
				return nil, err
			}
		}
	}

	return links, nil
}
