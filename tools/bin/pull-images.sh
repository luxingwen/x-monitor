#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

KUBE_VERSION=v1.24.1
KUBE_PAUSE_VERSION=3.7
ETCD_VERSION=3.5.3-0
DNS_VERSION=v1.8.0

GCR_URL=k8s.gcr.io
## k8smx这个你可以换
DOCKERHUB_URL=k8smx

images=(
kube-proxy:${KUBE_VERSION}
kube-scheduler:${KUBE_VERSION}
kube-controller-manager:${KUBE_VERSION}
kube-apiserver:${KUBE_VERSION}
pause:${KUBE_PAUSE_VERSION}
etcd:${ETCD_VERSION}
coredns:${DNS_VERSION}
)

for imageName in ${images[@]} ; do
  crictl pull $DOCKERHUB_URL/$imageName
  docker tag $DOCKERHUB_URL/$imageName $GCR_URL/$imageName
  docker rmi $DOCKERHUB_URL/$imageName
done