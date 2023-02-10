/*
 * @Author: CALM.WU
 * @Date: 2023-02-10 10:33:45
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2023-02-10 11:03:44
 */

package netutil

import (
	"context"
	"errors"
	"net/http"
	"net/http/httputil"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/golang/glog"
	calmutils "github.com/wubo0067/calmwu-go/utils"
)

// WebSrv contains webserver resource
type WebSrv struct {
	srvName  string
	ctx      context.Context
	httpSrv  *http.Server
	router   *gin.Engine
	bindAddr string
}

// ginLogger is a middleware that logs the request as it goes in and the response as it goes out.
func ginLogger(c *gin.Context) {
	t := time.Now()
	c.Next()
	latency := time.Since(t)
	glog.V(5).Infof("%s latency:%s", c.Request.RequestURI, latency.String())
}

// This function is used to recover from panics that occur in a gin handler.
// If a panic occurs, the recovery function logs the panic, and aborts the request with a 500 status.
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

// NewWebSrv make a websrv instance
func NewWebSrv(name string, ctx context.Context, bindAddr string, useTLS bool, certFile, keyFile string) *WebSrv {
	tmpRouter := gin.New()
	tmpRouter.Use(ginLogger)
	tmpRouter.Use(ginRecover)
	tmpRouter.Use(CORSMiddleware())

	webSrv := &WebSrv{
		srvName: name,
		ctx:     ctx,
		router:  tmpRouter,
		httpSrv: &http.Server{
			Addr:    bindAddr,
			Handler: tmpRouter,
		},
	}

	return webSrv
}

// Start listen for websocket
func (ws *WebSrv) Start() {
	go func() {
		glog.Infof("Starting up %s %s", ws.srvName, ws.httpSrv.Addr)
		if err := ws.httpSrv.ListenAndServe(); err != nil && errors.Is(err, http.ErrServerClosed) {
			glog.Errorf("Shutdown [%s] ==> %s\n", ws.srvName, err)
		} else {
			glog.Fatalf("%s ListenAndServe failed. error: %s", ws.srvName, err.Error())
		}
	}()
}

// Stop, shutdown websocket services
func (ws *WebSrv) Stop() {
	if ws.httpSrv != nil {
		_ = ws.httpSrv.Shutdown(ws.ctx)
		glog.Infof("%s shutdown now", ws.srvName)
	}
}

// Handle register a new request handle
func (ws *WebSrv) Handle(httpMethod string, relativePath string, handlers ...gin.HandlerFunc) {
	ws.router.Handle(httpMethod, relativePath, handlers...)
}
