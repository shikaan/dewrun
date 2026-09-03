#include <assert.h>
#include "src/cli.h"
#include "src/config.h"
#include "src/log.h"
#include "src/ui.h"
#include "src/shell.h"

static shl_callbacks_t callbacks = {
    .draw = NULL,
    .key = NULL,
    .pointer = NULL,
};

int main(int argc, char *const *argv) {
  static config_t *config = NULL;

  cli_opts_t *cli_opts;
  cli_parse(argc, argv, &cli_opts);
  assert(cli_opts && "cli_opts must be non-null");

  log_init(cli_opts->debug ? LOG_LEVEL_DEBUG : LOG_LEVEL_INFO);
  log_debug("cli options: configuration=%s",
            cli_opts->config ? cli_opts->config : "(null)");
  log_debug("cli options: debug=%s", cli_opts->debug ? "true" : "false");

  const char *config_path = cli_opts->config ? cli_opts->config : cfg_path();
  cfg_read(config_path, &config);
  assert(config && "config must be non-null");

  if (cli_opts->debug) {
    cfg_debug();
  }

  if (ui_init() != OK)
    return 1;

  shl_shell_t *shl = NULL;
  if (shl_create(callbacks, &shl) != OK)
    return 1;

  assert(shl && "must be non-null");
  return shl_run() == OK ? 0 : 1;
}
