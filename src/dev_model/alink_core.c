/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#include "iotx_alink_internal.h"
#include "alink_wrapper.h"

#include "alink_core.h"

#include "infra_defs.h"


#define ALINK_SUPPORT_BEARER_NUM                (1)
#define ALINK_DEFAULT_BEARER_PROTOCOL           (ALINK_BEARER_MQTT)




typedef enum {
    ALINK_DEV_STATUS_UNAUTHORIZED,    /* Subdev Created */
    ALINK_DEV_STATUS_AUTHORIZED,      /* Receive Topo Add Notify */
    ALINK_DEV_STATUS_REGISTERED,      /* Receive Subdev Registered */
    ALINK_DEV_STATUS_ATTACHED,        /* Receive Subdev Topo Add Reply */
    ALINK_DEV_STATUS_LOGINED,         /* Receive Subdev Login Reply */
    ALINK_DEV_STATUS_ONLINE,          /* After All Topic Subscribed */
    ALINK_DEV_STATUS_ENABLE,          /* enabled */
    ALINK_DEV_STATUS_DISABLE          /* disabled by cloud */
} alink_device_status_t;


typedef struct {
    uint32_t devid;
    char product_key[IOTX_PRODUCT_KEY_LEN];
    char device_name[IOTX_DEVICE_NAME_LEN];
    char device_secret[IOTX_DEVICE_SECRET_LEN];
    alink_device_status_t status;
    list_head_t list;
} alink_subdev_node_t;


typedef struct {
    uint8_t                 is_inited;
    uint8_t                 bearer_num;
    void                   *mutex;

    alink_bearer_node_t    *p_activce_bearer;
    uint32_t                msgid;              // TODO


    uint16_t                subdev_num;
    uint32_t                devid_alloc;
    list_head_t             dev_list;           // still use a huge list, TODO
} alink_core_ctx_t;

static alink_core_ctx_t alink_core_ctx = { 0 };

/****************
 *
 *******************/
static void _alink_core_lock(void)
{
    if (alink_core_ctx.mutex) {
        HAL_MutexLock(alink_core_ctx.mutex);
    }
}

static void _alink_core_unlock(void)
{
    if (alink_core_ctx.mutex) {
        HAL_MutexUnlock(alink_core_ctx.mutex);
    }
}


/************************
 * 
 ************************/

uint32_t alink_core_get_msgid(void)
{
    uint32_t msgid;

    _alink_core_lock();
    msgid = alink_core_ctx.msgid++;
    _alink_core_unlock();

    return (msgid & 0x7FFFFFFF);
}

int alink_core_init(void)
{
    int res = FAIL_RETURN;

    if (alink_core_ctx.is_inited) {
        return res;
    }

    alink_core_ctx.is_inited = 1;
    alink_core_ctx.mutex = HAL_MutexCreate();
    if (alink_core_ctx.mutex == NULL) {
        alink_core_ctx.is_inited = 0;
        return res;
    }

    res = alink_downstream_hash_table_init();
    if (res == FAIL_RETURN) {
        HAL_MutexDestroy(alink_core_ctx.mutex);
        return res;
    }

    return SUCCESS_RETURN;
}

int alink_core_deinit(void)
{
    if (!alink_core_ctx.is_inited) {
        return FAIL_RETURN;
    }

    alink_core_ctx.is_inited = 0;
    HAL_MutexDestroy(alink_core_ctx.mutex);

    return SUCCESS_RETURN;
}

/**
 *
 */
int alink_core_open(iotx_dev_meta_info_t *dev_info)
{
    ALINK_ASSERT_DEBUG(dev_info != NULL);

    int res = alink_core_init();
    if (res == FAIL_RETURN) {
        return res;
    }

    if (alink_core_ctx.bearer_num >= ALINK_SUPPORT_BEARER_NUM) {
        return FAIL_RETURN;
    }
    alink_core_ctx.bearer_num++;

    /* add one default bearer protocol */
    alink_core_ctx.p_activce_bearer = alink_bearer_open(ALINK_DEFAULT_BEARER_PROTOCOL, dev_info);
    if (alink_core_ctx.p_activce_bearer == NULL) {
        alink_info("bearer open fail");
        alink_core_deinit();        // TODO
        return FAIL_RETURN;
    }

    /* TOOD */
    _alink_core_lock();
    _alink_core_unlock();

    alink_info("bearer open succeed");
    return SUCCESS_RETURN;
}


int alink_core_close(void)
{


    return SUCCESS_RETURN;
}


static uint32_t alink_subdev_allocate_devid(void)
{
    uint32_t devid;

    _alink_core_lock();

    devid = ++alink_core_ctx.devid_alloc;

    _alink_core_unlock();

    return devid;
}

/**
 * move from dm
 */
static int alink_subdev_search_node_by_pkdn(const char *product_key, const char *device_name, alink_subdev_node_t **node)
{
    alink_core_ctx_t *ctx = &alink_core_ctx;        // TODO
    alink_subdev_node_t *search_node = NULL;

    list_for_each_entry(search_node, &ctx->dev_list, list, alink_subdev_node_t) {
        if ((strlen(search_node->product_key) == strlen(product_key)) &&
            (memcmp(search_node->product_key, product_key, strlen(product_key)) == 0) &&
            (strlen(search_node->device_name) == strlen(device_name)) &&
            (memcmp(search_node->device_name, device_name, strlen(device_name)) == 0)) {
            alink_info("Device Found, Product Key: %s, Device Name: %s", product_key, device_name);
            if (node) {
                *node = search_node;
            }
            return SUCCESS_RETURN;
        }
    }

    alink_info("Device Not Found, Product Key: %s, Device Name: %s", product_key, device_name);
    return FAIL_RETURN;
}

/**
 * move from dm
 */
static int alink_subdev_search_node_by_devid(uint32_t devid, alink_subdev_node_t **node)
{
    alink_core_ctx_t *ctx = &alink_core_ctx;        // TODO
    alink_subdev_node_t *search_node = NULL;

    list_for_each_entry(search_node, &ctx->dev_list, list, alink_subdev_node_t) {
        if (search_node->devid == devid) {
            alink_info("Device Found, devid: %d", devid);
            if (node) {
                *node = search_node;
            }
            return SUCCESS_RETURN;
        }
    }

    alink_info("Device Not Found, devid: %d", devid);
    return FAIL_RETURN;
}

/**
 * 
 */
int alink_subdev_create(const char *product_key, const char *device_name, const char *device_secret, int *devid)
{
    int res = FAIL_RETURN;
    alink_core_ctx_t *ctx = &alink_core_ctx;
    alink_subdev_node_t *node = NULL;

    ALINK_ASSERT_DEBUG(product_key != NULL);
    ALINK_ASSERT_DEBUG(device_name != NULL);
    ALINK_ASSERT_DEBUG(devid != NULL);

    res = alink_subdev_search_node_by_pkdn(product_key, device_name, &node);
    if (res == SUCCESS_RETURN) {
        // TOOD: already exist
        return FAIL_RETURN;
    }

    node = HAL_Malloc(sizeof(alink_subdev_node_t));
    if (node == NULL) {
        return ALINK_CODE_MEMORY_NOT_ENOUGH;
    }
    memset(node, 0, sizeof(alink_subdev_node_t));

    node->devid = alink_subdev_allocate_devid();
    *devid = node->devid;

    memcpy(node->product_key, product_key, strlen(product_key));
    memcpy(node->device_name, device_name, strlen(device_name));
    if (device_secret != NULL) {
        memcpy(node->device_secret, device_secret, strlen(device_secret));
    }

    node->status = ALINK_DEV_STATUS_AUTHORIZED;
    INIT_LIST_HEAD(&node->list);
    list_add_tail(&node->list, &ctx->dev_list);

    return SUCCESS_RETURN;
}

int alink_subdev_destroy(uint32_t devid)
{
    ALINK_ASSERT_DEBUG(devid > 0);

    int res = 0;

    alink_subdev_node_t *node = NULL;

    res = alink_subdev_search_node_by_devid(devid, &node);
    if (res != SUCCESS_RETURN) {
        return FAIL_RETURN;
    }

    list_del(&node->list);
    HAL_Free(node);

    return SUCCESS_RETURN;
}



int alink_subdev_search_dev_by_devid(int devid, char *product_key, char *device_name, char *device_secret)
{
    ALINK_ASSERT_DEBUG(devid > 0);

    int res = 0;
    alink_subdev_node_t *node = NULL;

    res = alink_subdev_search_node_by_devid(devid, &node);
    if (res != SUCCESS_RETURN) {
        return FAIL_RETURN;
    }

    if (product_key != NULL) {
        memcpy(product_key, node->product_key, strlen(node->product_key));
    }
    
    if (device_name != NULL) {
        memcpy(device_name, node->device_name, strlen(node->device_name));
    }
    
    if (device_secret != NULL) {
        memcpy(device_secret, node->device_secret, strlen(node->device_secret));
    }

    return SUCCESS_RETURN;
}

int alink_subdev_search_dev_by_pkdn(const char *product_key, const char *device_name, int *devid)
{
    ALINK_ASSERT_DEBUG(product_key != NULL);
    ALINK_ASSERT_DEBUG(device_name != NULL);
    ALINK_ASSERT_DEBUG(devid != NULL);

    int res = 0;
    alink_subdev_node_t *node = NULL;

    res = alink_subdev_search_node_by_pkdn(product_key, device_name, &node);
    if (res != SUCCESS_RETURN) {
        return FAIL_RETURN;
    }

    *devid = node->devid;

    return SUCCESS_RETURN;
}









int alink_core_subdev_init()
{
    int res = FAIL_RETURN;

    return res;
}


int alink_subdev_open(iotx_dev_meta_info_t *dev_info)
{
    if (alink_core_ctx.is_inited == IOT_FALSE) {
        return FAIL_RETURN;
    }




    return SUCCESS_RETURN;
}

int alink_core_connect_cloud(void)
{
    int res = FAIL_RETURN;

    res = alink_bearer_conect();
    if (res == FAIL_RETURN) {
        return res;
    }

    alink_debug("start sub");
    alink_core_subscribe_downstream();

    return res;
}

int alink_core_send_req_msg(char *uri, uint8_t *payload, uint32_t len)
{
    ALINK_ASSERT_DEBUG(uri != NULL);

    int res = FAIL_RETURN;

    /* append query */
    alink_info("uri: %s", uri);

    alink_bearer_send(0, uri, payload, len);

    HAL_Free(uri);

    return res;
}

int alink_core_subdev_connect_cloud(uint32_t devid)
{
    int res = FAIL_RETURN;

    return res;
}

/**
 *
 */
void _alink_core_rx_event_handle(void *handle, const char *uri, uint32_t uri_len, const char *payload, uint32_t payload_len)
{   
    /* assert */

    alink_uri_query_t query = { 0 };
    char product_key[IOTX_PRODUCT_KEY_LEN] = {0};
    char device_name[IOTX_DEVICE_NAME_LEN] = {0};
    char path[50] = {0};        // TODO: len?
    uint8_t is_subdev;

    alink_downstream_handle_func_t handle_func;

    /* reslove the uri query */
    alink_format_reslove_uri(uri, uri_len, product_key, device_name, path, &query, &is_subdev);

    handle_func = alink_downstream_get_handle_func(path, strlen(path));

    /* reslove the uri path */
    alink_info("rx data, uri = %.*s", uri_len, uri);
    alink_info("rx data, data = %.*s", payload_len, payload);

    alink_info("pk = %s", product_key);
    alink_info("dn = %s", device_name);
    alink_info("path = %s", path);
    alink_info("is_subdev = %d", is_subdev);

    alink_info("query id = %d", query.id);
    alink_info("query format = %c", query.format);
    alink_info("query compress = %c", query.compress);
    alink_info("query code = %d", query.code);
    alink_info("query ack = %c", query.ack);


    // TODO: for test!!!
    if (handle_func != NULL) {
        handle_func(product_key, device_name, (uint8_t *)payload, payload_len, &query);
    }
    else {
        alink_info("rx handle func is NULL");
    }

}

/**
 *
 */
int alink_core_subscribe_downstream(void)
{
    int res = FAIL_RETURN;

    const char *uri = "/sys/a1OFrRjV8nz/develop_01/thing/service/property/set";



    res = alink_bearer_register(alink_core_ctx.p_activce_bearer, uri, (alink_bearer_rx_cb_t)_alink_core_rx_event_handle);

    return res;
}

int alink_core_unsubscribe_downstream_uri()
{
    int res = FAIL_RETURN;

    return res;
}

/**
 *
 */
int alink_core_yield(uint32_t timeout_ms)
{
    return alink_bearer_yield(timeout_ms);
}

int alink_core_subdev_deinit()
{
    int res = FAIL_RETURN;

    return res;
}
