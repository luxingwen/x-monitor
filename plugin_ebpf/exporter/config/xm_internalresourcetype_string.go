// Code generated by "stringer -type=XMInternalResourceType -output=xm_internalresourcetype_string.go"; DO NOT EDIT.

package config

import "strconv"

func _() {
	// An "invalid array index" compiler error signifies that the constant values have changed.
	// Re-run the stringer command to generate them again.
	var x [1]struct{}
	_ = x[XM_Resource_None-0]
	_ = x[XM_Resource_OS-1]
	_ = x[XM_Resource_Namespace-2]
	_ = x[XM_Resource_Cgroup-3]
	_ = x[XM_Resource_PID-4]
	_ = x[XM_Resource_PGID-5]
}

const _XMInternalResourceType_name = "XM_Resource_NoneXM_Resource_OSXM_Resource_NamespaceXM_Resource_CgroupXM_Resource_PIDXM_Resource_PGID"

var _XMInternalResourceType_index = [...]uint8{0, 16, 30, 51, 69, 84, 100}

func (i XMInternalResourceType) String() string {
	if i < 0 || i >= XMInternalResourceType(len(_XMInternalResourceType_index)-1) {
		return "XMInternalResourceType(" + strconv.FormatInt(int64(i), 10) + ")"
	}
	return _XMInternalResourceType_name[_XMInternalResourceType_index[i]:_XMInternalResourceType_index[i+1]]
}
