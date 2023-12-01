FROM alpine

# 变量重新声明，必须如此
#ARG app

WORKDIR /x-monitor
RUN mkdir -p bin
RUN mkdir -p config

COPY ../../bin/plugins.d/x-monitor.ebpf /x-monitor/bin/
COPY ../../env/config/xm_ebpf_plugin/config.yaml /x-monitor/config/

RUN chmod +x /x-monitor/bin/* && ls -R ./ && pwd

ENTRYPOINT ["/x-monitor/bin/x-monitor.ebpf"]
CMD ["--config=/x-monitor/config/config.yaml", "--pyroscope=http://127.0.0.1:4040/", "--log_dir=/var/log", "--logtostderr=true", "--v=5", ">/dev/null", "2>&1"]
