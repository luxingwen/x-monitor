APP := x-monitor.eBPF

GIT_COMMIT_HASH ?= $(shell git rev-parse --short HEAD)
GIT_BRANCH_NAME ?= $(shell git symbolic-ref --short -q HEAD)
BUILD_TIME ?= $(shell date -u '+%Y-%m-%d_%H:%M:%S')
VersionMajor ?= 1
VersionMinor ?= 0

WORKDIR=$(shell pwd)
ADDX=$(shell sh -c chmod +x ./hack/*)
PACKAGES=`go list ./... | grep -v /vendor/`
GOFILES=$(shell find . -name "*.go" -type f -not -path "./vendor/*" -not -path "./collector/bpfmodule/*")

GOARCH ?= $(shell go env GOARCH)
GOOS ?= $(shell go env GOOS)
GO_VERSION := $(shell go version)

.PHONY: all build image clean lint test swagger upload-images deps build-version validation generate
.IGNORE : lint

all: build

build:
	#swag i
	gofmt -s -w $(GOFILES)
	goimports -v -w $(GOFILES)
	go generate -v -x ./...
	GOOS=linux GOARCH=amd64 go build -mod=vendor -pgo=auto  -v -x \
		-ldflags '-X xmonitor.calmwu/plugin_ebpf/collector/cmd.VersionMajor=$(VersionMajor) -X xmonitor.calmwu/plugin_ebpf/collector/cmd.VersionMinor=$(VersionMinor) -X xmonitor.calmwu/plugin_ebpf/collector/cmd.CommitHash=$(GIT_COMMIT_HASH) -X xmonitor.calmwu/plugin_ebpf/collector/cmd.BranchName=$(GIT_BRANCH_NAME) -X xmonitor.calmwu/plugin_ebpf/collector/cmd.BuildTime=$(BUILD_TIME) -linkmode external' \
		-gcflags 'all=-N -l' -race  \
		-o ../../bin/plugins.d/$(APP) main.go

test:
	go test --race -v ./...

fieldalignment:
	#go vet -vettool=$(which fieldalignment) ./...
	fieldalignment -fix -json ./...

fmt:
	gofmt -s -w $(GOFILES)

lint:
	docker run --rm -v ${WORKDIR}:/src/exporter -w /src/exporter golangci/golangci-lint:v1.40.1-alpine \
		golangci-lint run -v --config=.golangci.yml ./...

build-version:
	@echo ------------ Start Build Version Details ------------
	@echo AppName: ${APP}
	@echo Arch: ${GOARCH}
	@echo OS: ${GOOS}
	@echo Version: ${VersionMajor}.{VersionMinor}
	@echo BuildTime: ${BUILD_TIME}
	@echo GoVersion: ${GO_VERSION}
	@echo CommitID: ${GIT_COMMIT_HASH}
	@echo BranchName : ${GIT_BRANCH_NAME}
	@echo ------------ End   Build Version Details ------------

swagger:
	swag i

generate:
	go generate -v -x ./...

clean:
	-rm ../../bin/plugins.d/$(APP)
	-cd collector/bpfmodule && ls | grep -v generate.go | xargs -I{} rm -f {} && cd -
