
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <assert.h>

#include "utils.h"

int main(int argc, char const *const *argv) {
    char const *hostname = "localhost";
    int port = 5672;
    const char *user = "test";
    const char *password = "test";
    int status;
    char const *exchange = "amq.direct";
    char const *bindingkey = "testkey";
    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;
    amqp_rpc_reply_t reply;
    //amqp_bytes_t queuename = amqp_cstring_bytes("queue01");
    amqp_bytes_t queuename;

    conn = amqp_new_connection();

    socket = amqp_tcp_socket_new(conn);
    if (!socket) {
	    printf("creating TCP socket failed");\
		    return -1;
    }
    printf("create TCP socket success\n");
    status = amqp_socket_open(socket, hostname, port);
    if (status) {
	    printf("opening TCP socket faild\n");
	    return -1;
    }
    printf("opening TCP socket success\n");
    reply = amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, user, password);
    if (reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION)
    {
        printf("amqp login error\n");
        return 0;
    }
    printf("amqp login success\n");

    amqp_channel_open(conn, 1);
    reply = amqp_get_rpc_reply(conn);
    if(reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION)
    {
        printf("open channel error\n");
        return -1;
    }
    printf("open channel success\n");
    
    amqp_queue_declare_ok_t *r = amqp_queue_declare(
		    conn, 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
    reply = amqp_get_rpc_reply(conn);
    if(reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION)
    {
        printf("Declaring queue error\n");
        return -2;           
    }       
    printf("Declare queue success\n");
    queuename = amqp_bytes_malloc_dup(r->queue);
    if (queuename.bytes == NULL) {
	    fprintf(stderr, "Out of memory while copying queue name");
	    return -1;
    }

    amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange),
		    amqp_cstring_bytes(bindingkey), amqp_empty_table);
    reply = amqp_get_rpc_reply(conn);
    if(reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION)
    {
        printf("bind queue error\n");
        return -4;
    }
    printf("bind queue success\n");
    amqp_basic_consume(conn, 1, queuename, amqp_empty_bytes, 0, 1, 0,
		    amqp_empty_table);
    reply = amqp_get_rpc_reply(conn);
    if(reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION)
    {
        printf("consuming error\n");
        return -4;
    }
    printf("consume success\n");
    for (;;) {
	    amqp_rpc_reply_t res;
	    amqp_envelope_t envelope;

	    amqp_maybe_release_buffers(conn);

	    res = amqp_consume_message(conn, &envelope, NULL, 0);

	    if (AMQP_RESPONSE_NORMAL != res.reply_type) {
		    break;
	    }

	    printf("Delivery %u, exchange %.*s routingkey %.*s\n",
			    (unsigned)envelope.delivery_tag, (int)envelope.exchange.len,
			    (char *)envelope.exchange.bytes, (int)envelope.routing_key.len,
			    (char *)envelope.routing_key.bytes);

	    if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
		    printf("Content-type: %.*s\n",
				    (int)envelope.message.properties.content_type.len,
				    (char *)envelope.message.properties.content_type.bytes);
	    }
	    printf("----\n");

	    amqp_dump(envelope.message.body.bytes, envelope.message.body.len);

	    amqp_destroy_envelope(&envelope);
    }

    amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(conn);
    return 0;
}

