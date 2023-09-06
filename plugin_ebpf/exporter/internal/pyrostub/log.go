/*
 * @Author: CALM.WU
 * @Date: 2023-09-06 10:46:58
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-09-06 14:21:38
 */

package pyrostub

import (
	"fmt"
	"strings"

	"github.com/golang/glog"
)

type loggerAdapter struct{}

var (
	__defaultLogger = &loggerAdapter{}
)

func (l *loggerAdapter) Log(keyvals ...interface{}) error {
	var msg strings.Builder

	for i := 0; i < len(keyvals); i += 2 {
		msg.WriteString(fmt.Sprintf("%s=%v ", keyvals[i].(string), keyvals[i+1]))
	}

	glog.Infof("%s", strings.TrimRight(msg.String(), " "))
	return nil
}
