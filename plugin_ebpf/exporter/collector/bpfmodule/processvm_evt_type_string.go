// Code generated by "stringer -type=XMProcessVMXmProcessvmEvtType -output=processvm_evt_type_string.go"; DO NOT EDIT.

package bpfmodule

import "strconv"

func _() {
	// An "invalid array index" compiler error signifies that the constant values have changed.
	// Re-run the stringer command to generate them again.
	var x [1]struct{}
	_ = x[XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_NONE-0]
	_ = x[XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_ANON_PRIV-1]
	_ = x[XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_SHARED-2]
	_ = x[XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_OTHER-3]
	_ = x[XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_BRK-4]
	_ = x[XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_BRK_SHRINK-5]
	_ = x[XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MUNMAP-6]
}

const _XMProcessVMXmProcessvmEvtType_name = "XMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_NONEXMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_ANON_PRIVXMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_SHAREDXMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MMAP_OTHERXMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_BRKXMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_BRK_SHRINKXMProcessVMXmProcessvmEvtTypeXM_PROCESSVM_EVT_TYPE_MUNMAP"

var _XMProcessVMXmProcessvmEvtType_index = [...]uint16{0, 55, 120, 182, 243, 297, 358, 415}

func (i XMProcessVMXmProcessvmEvtType) String() string {
	if i >= XMProcessVMXmProcessvmEvtType(len(_XMProcessVMXmProcessvmEvtType_index)-1) {
		return "XMProcessVMXmProcessvmEvtType(" + strconv.FormatInt(int64(i), 10) + ")"
	}
	return _XMProcessVMXmProcessvmEvtType_name[_XMProcessVMXmProcessvmEvtType_index[i]:_XMProcessVMXmProcessvmEvtType_index[i+1]]
}