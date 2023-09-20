# Profile

## 搭建环境

1. 安装podman

   ```
   dnf -y install podman-docker
   ```

2. 创建网络namespace

   ```
   docker network create xm-pyroscope 
   ```

3. 安装grafana

   ```
   docker pull grafana/grafana:main
   docker run --rm --name=grafana -d -p 3000:3000 -e "GF_FEATURE_TOGGLES_ENABLE=flameGraph" --network=xm-pyroscope  grafana/grafana:main
   ```

4. 安装pyroscope

   ```
   docker pull grafana/pyroscope:1.0.0
   docker run --rm --name pyroscope -d --network=xm-pyroscope -p 4040:4040 --network=xm-pyroscope grafana/pyroscope:1.0.0
   ```

   二进制启动

5. 配置

   - 查看容器ip

     ```
     docker exec -it 7738b5e2ee19 ip addr
     ```

     

## 安装x-monitor.eBPF

1. 配置
2. 启动

## 测试

1. 配置grafana-agent-flow

   ```
   logging {
   	level = "debug"
   }
   
   pyroscope.ebpf "instance" {
   	forward_to = [pyroscope.write.endpoint.receiver]
   	default_target = {
   		"__name__"="host_cpu",
   		"service_name"="host_cpu", 
   	}
           targets = [
   		{"__container_id__"="4ec0624267de013f5b936c7bc37a6551d95a997fa659d41a9f840e7d32b84378", 
   		 "service_name"="grafana", 
   		 "__name__"="grafana_cpu"},
   	]
   	targets_only = false
   	collect_kernel_profile = true
   	collect_user_profile = true
   	collect_interval = "5s"
   	sample_rate = 100
   }
   
   pyroscope.write "endpoint" {
   	endpoint {
           url = "http://10.0.0.4:4040"
       }
   }
   
   ```

2. 启动grafana-agent-flow

   ```
   ./grafana-agent-flow run /home/pingan/Program/agent/build/grafana-agent-flow.river
   ```

## 观察

## 资料

1. [Get started with Pyroscope | Grafana Pyroscope documentation](https://grafana.com/docs/pyroscope/latest/get-started/)

