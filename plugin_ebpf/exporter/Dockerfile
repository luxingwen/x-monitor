FROM debian:bookworm-slim

# 变量重新声明，必须如此
#ARG app

WORKDIR /x-monitor
RUN mkdir -p bin
RUN mkdir -p config
RUN mkdir -p log

#将宿主机的执行程序复制到容器中，都在WORKDIR指定目录下
COPY ../../bin/plugins.d/x-monitor.ebpf ./bin/
COPY ../../env/config/xm_ebpf_plugin/config.yaml ./config/

RUN chmod +x ./bin/* && ls -R ./ && pwd
USER 0:0
ENTRYPOINT ["./bin/x-monitor.ebpf", "--config=./config/config.yaml", "--log_dir=./log", "--logtostderr=true", "--v=5", "2>&1"]
CMD ["--pyroscope=http://127.0.0.1:4040/"]
