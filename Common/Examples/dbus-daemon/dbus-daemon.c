/*
 * This is an example Linux daemon that communicates via dbus.
 * When run, it will daemonize and print to standard output two lines:
 *
 *   1. The PID of the daemon process, which can be used to kill it later with `kill -s SIGINT <pid>'
 *   2. The unique D-Bus address it will be listening to
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <gio/gio.h>

static const gchar introspection_xml[] =
	"<node>"
	"  <interface name='dradtke.DBusDaemon'>"
	"    <method name='SendMessage'>"
	"      <arg type='s' name='message' direction='in'/>"
	"      <arg type='s' name='response' direction='out'/>"
	"    </method>"
	"  </interface>"
	"</node>";

static GMainLoop *loop;
static GDBusConnection *connection;
static int pipefd[2];

/*
 * Handle method calls
 */
static void handle_method_call(GDBusConnection *conn,
                               const gchar *sender,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *method_name,
                               GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data)
{
	if (!g_strcmp0(method_name, "SendMessage")) {
		gchar *message;
		gchar *response;
		g_variant_get(parameters, "(s)", &message);
		response = g_strdup_printf("Received message: %s", message);
		g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", response));
		g_free(message);
		g_free(response);
	}
}

/*
 * Handle property queries
 */
static GVariant *handle_get_property(GDBusConnection *conn,
                                     const gchar *sender,
                                     const gchar *object_path,
                                     const gchar *interface_name,
                                     const gchar *property_name,
                                     GError **error,
                                     gpointer user_data)
{
	return NULL;
}

/*
 * Handle property modifications
 */
static gboolean handle_set_property(GDBusConnection *conn,
                                    const gchar *sender,
                                    const gchar *object_path,
                                    const gchar *interface_name,
                                    const gchar *property_name,
                                    GVariant *value,
                                    GError **error,
                                    gpointer user_data)
{
	return FALSE;
}

/*
 * Function vtable for handling methods and properties
 */
static const GDBusInterfaceVTable interface_vtable = {
	&handle_method_call,
	&handle_get_property,
	&handle_set_property
};

/*
 * Close our association with syslog before exiting
 */
static void cleanup()
{
	closelog();
}

/*
 * Forks the current process and tells the parent to quit
 */
static void daemonize(char *name)
{
	pid_t pid, sid;
	
	// fork off the original process and tell the parent to quit
	pid = fork();
	if (pid != 0) {
		if (pid == -1) {
			close(pipefd[0]);
			close(pipefd[1]);
			exit(EXIT_FAILURE);
		} else {
			close(pipefd[1]);
			printf("%d\n", pid);
			char buf[32];
			while (read(pipefd[0], buf, sizeof(buf)) > 0) {
				printf("%s", buf);
			}
			printf("\n");
			close(pipefd[0]);
			exit(EXIT_SUCCESS);
		}
	}

	// running as the child, continue daemonizing
	close(pipefd[0]);
	umask(0);
	openlog(name, LOG_NOWAIT|LOG_PID, LOG_USER);
	atexit(&cleanup);
	syslog(LOG_NOTICE, "Starting daemon...\n");

	// try to create a process group
	if ((sid = setsid()) < 0) {
		syslog(LOG_ERR, "Could not create process group\n");
		exit(EXIT_FAILURE);
	}

	if ((chdir("/")) < 0) {
		syslog(LOG_ERR, "Could not switch to root directory\n");
		exit(EXIT_FAILURE);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}

/*
 * On SIGINT, exit the main loop
 */
static void sig_handler(int signo)
{
	if (signo == SIGINT) {
		g_main_loop_quit(loop);
	}
}

int main(int argc, char *argv[])
{
	if (pipe(pipefd) < 0) {
		exit(EXIT_FAILURE);
	}

	daemonize(argv[0]);

	// set up the SIGINT signal handler
	if (signal(SIGINT, &sig_handler) == SIG_ERR) {
		syslog(LOG_ERR, "Failed to register SIGINT handler, quitting...\n");
		exit(EXIT_FAILURE);
	}

	g_type_init();
	loop = g_main_loop_new(NULL, FALSE);

	// get the bus connection and print out its unique name
	GError *error = NULL;
	connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	if (error != NULL) {
		syslog(LOG_ERR, "Failed to acquire connection, quitting...\n");
		close(pipefd[1]);
		exit(EXIT_FAILURE);
	} else {
		gchar *conn_name;
		g_object_get(connection, "unique-name", &conn_name, NULL);
		write(pipefd[1], conn_name, strlen(conn_name) + 1);
		close(pipefd[1]);
		g_free(conn_name);
	}

	// register our object
	GDBusNodeInfo *introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
	GDBusInterfaceInfo *interface_info = g_dbus_node_info_lookup_interface(introspection_data, "dradtke.DBusDaemon");
	g_dbus_connection_register_object(connection,
	                                  "/dradtke/DBusDaemon",
	                                  interface_info,
	                                  &interface_vtable,
	                                  NULL,
	                                  NULL,
	                                  NULL);

	// start the loop
	syslog(LOG_NOTICE, "Entering main loop");
	g_main_loop_run(loop);

	syslog(LOG_NOTICE, "Exiting...");
	g_main_loop_unref(loop);
	return 0;
}

