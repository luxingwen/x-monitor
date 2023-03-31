# 定义shell函数
define myfunc
    ls -al
endef

# 定义变量
FILES := $(shell $(call myfunc,txt))

# 使用变量
all:
    #@echo $(FILES)
	@echo $(shell $(call myfunc,txt))
