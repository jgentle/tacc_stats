/* Adapted from rabbitmq-c examples/amqp_listen.c */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>
#include <assert.h>

#include "string1.h"
#include "trace.h"

int consume(const char *hostname, const char* port, const char* archive_dir)
{

  int rc = 0;
  int status;
  char const *exchange;
  char const *bindingkey;
  amqp_socket_t *socket = NULL;
  amqp_connection_state_t conn;
  amqp_bytes_t queuename;

  exchange = "amq.direct";
  bindingkey = "tacc_stats";

  conn = amqp_new_connection();
  socket = amqp_tcp_socket_new(conn);
  status = amqp_socket_open(socket, hostname, atoi(port));
  amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest");
  amqp_channel_open(conn, 1);
  amqp_get_rpc_reply(conn);

  {
    amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, 1, amqp_cstring_bytes("tacc_stats"), 0, 1, 0, 0,
                                 amqp_empty_table);
    amqp_get_rpc_reply(conn);
    queuename = amqp_bytes_malloc_dup(r->queue);
    if (queuename.bytes == NULL) {
      fprintf(stderr, "Out of memory while copying queue name");
      goto out;
    }
  }

  amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange), amqp_cstring_bytes(bindingkey),
                  amqp_empty_table);
  amqp_get_rpc_reply(conn);

  amqp_basic_consume(conn, 1, queuename, amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
  amqp_get_rpc_reply(conn);

  // Write data to file in hostname directory
  FILE *fd;

  {
    while (1) {
      amqp_rpc_reply_t res;
      amqp_envelope_t envelope;

      amqp_maybe_release_buffers(conn);

      res = amqp_consume_message(conn, &envelope, NULL, 0);
      status = amqp_basic_ack(conn, 1, envelope.delivery_tag, 0);

      if (AMQP_RESPONSE_NORMAL != res.reply_type) {
        break;
      }
      umask(022);

      char *data_buf;
      asprintf(&data_buf, "%s", (char *) envelope.message.body.bytes);

      char *line, *hostname;
      line = wsep(&data_buf);

      int new_file = 0;

      if (*(line) == '$') {  // If schema get hostname and start new file
	while(1) {
	  line = wsep(&data_buf);
	  if (strcmp(line  ,"$hostname") == 0) {
	    hostname = wsep(&data_buf);
	    break;
	  }
	}
	new_file = 1;
      }
      else { // If stats data get hostname
	line = wsep(&data_buf);
	hostname = wsep(&data_buf);
      }	
 
      // Make directory for host hostname if it doesn't exist
      const char *stats_dir_path = strf("%s/%s",archive_dir,hostname);
      if (mkdir(stats_dir_path, 0777) < 0) {
	if (errno != EEXIST)
	  FATAL("cannot create directory `%s': %m\n", stats_dir_path);
      }
      char *current_path = strf("%s/%s",stats_dir_path,"current");

      // Unlink from old file if starting a new file      
      if (new_file) {
	if (unlink(current_path) < 0 && errno != ENOENT) {
	  ERROR("cannot unlink `%s': %m\n", current_path);
	  rc = 1;
	}
	fd = fopen(current_path, "w");
	struct timeval tp;
	double current_time;
	gettimeofday(&tp,NULL);
	current_time = tp.tv_sec+tp.tv_usec/1000000.0;

	// Link to new file which will be left behind after next rotation
	char *link_path = strf("%s/%ld", stats_dir_path, (long)current_time);
	if (link_path == NULL)
	  ERROR("cannot create path: %m\n");
	else if (link(current_path, link_path) < 0)
	  ERROR("cannot link `%s' to `%s': %m\n", current_path, link_path);
	free(link_path);	  	 
      }
      else {
	fd = fopen(current_path, "a+");
      }

      fprintf(fd,"%.*s",
	      (int) envelope.message.body.len, 
	      (char *) envelope.message.body.bytes);
      fflush(fd);
      fclose(fd);
      free(current_path);

      amqp_destroy_envelope(&envelope);
    }
  }
  
  amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
  amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
  amqp_destroy_connection(conn);

 out:
  return rc;
}

int main(int argc, char *argv[])
{

  char *hostname = NULL;
  char *archive_dir = NULL;
  char *port = NULL;
  int rc = 0;

  if (argc < 3) {
    fprintf(stderr, "Usage: command amqp_listend -s SERVER -a ARCHIVE_DIR -p PORT (5672 is default)\n");
    return 1;
  }

  struct option opts[] = {
    { "server", required_argument, 0, 's' },
    { "port", required_argument, 0, 'p' },
    { "archive_dir", required_argument, 0, 'a' },
    { NULL, 0, 0, 0 },
  };

  int c;
  while ((c = getopt_long(argc, argv, "s:p:a:", opts, 0)) != -1) {
    switch (c) {
    case 's':
      hostname = optarg;
      continue;
    case 'p':
      port = optarg;
      continue;
    case 'a':
      archive_dir = optarg;
      continue;
    }
  }

  if (hostname == NULL) {
    FATAL("Must specify a RMQ server with -s [--server] argument.\n");
  }
  if (archive_dir == NULL) {
    FATAL("Must specify an archive dir -a [--archive_dir] argument.\n");
  }
  if (port == NULL) {
    port = "5672";
  }
  consume(hostname, port, archive_dir);

  return rc;
}
