/*
 * @Author: CALM.WU
 * @Date: 2023-02-09 17:32:06
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-09 17:39:34
 */

package cmd

import (
	"context"
	"net/http"
	"net/http/httputil"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/golang/glog"

	calmutils "github.com/wubo0067/calmwu-go/utils"
)

type PromSrv struct {
	ctx     context.Context
	httpSrv *http.Server
	router  *gin.Engine
}

// ginLogger is a middleware that logs the request method and latency, and the response code.
func ginLogger(c *gin.Context) {
	t := time.Now()
	c.Next()
	latency := time.Since(t)
	glog.Infof("%s latency:%s", c.Request.RequestURI, latency.String())
}

// ginRecover recovers from panics and logs the request and error.
// It also returns a 500 status code to the user.
func ginRecover(c *gin.Context) {
	defer func() {
		if err := recover(); err != nil {
			stack := calmutils.CallStack(2)
			httprequest, _ := httputil.DumpRequest(c.Request, false)
			glog.Errorf("[Recovery] panic recovered:\n%s\n%s\n%s", calmutils.Bytes2String(httprequest), err, stack)
			c.AbortWithStatus(500)
		}
	}()
	c.Next()
}

func StartPromSrv() error {
	return nil
}

func StopPromSrv() {
}
