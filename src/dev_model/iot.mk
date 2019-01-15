LIBA_TARGET     := libiot_alink.a

HDR_REFS        += src/infra
HDR_REFS		+= src/mqtt
HDR_REFS		+= src/dev_sign

DEPENDS         += external_libs/refs wrappers
LDFLAGS         += -liot_sdk -liot_hal -liot_tls

TARGET          := alink-example-solo alink-example-gateway

LIB_SRCS_PATTERN     := *.c
LIB_SRCS_EXCLUDE     := alink_example_solo.c alink_example_gateway.c
SRCS_alink-example-solo   	+= alink_example_solo.c
SRCS_alink-example-gateway	+= alink_example_gateway.c
