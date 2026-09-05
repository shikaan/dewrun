#include "src/cli.h"
#include "src/config.h"
#include "src/ctx.h"
#include "src/log.h"
#include "src/result.h"
#include "src/shell.h"
#include "src/ui.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/////////////////////// Application state
typedef struct {
  char buf[1024];
  size_t len;
} input_t;

ui_font_t *text = NULL;

static input_t input = {0};

/////////////////////////
static config_t *config = NULL;

bool handle_key(shl_kbd_event_t evt, shl_key_t key, char *buf, size_t len) {
#define min(a, b) (a < b ? a : b)
  (void)key;
  (void)len;
  log_debug("input.len = %u", input.len);
  if (evt == SHL_KBD_EVENT_KEYDOWN) {
    if (key == SHL_KEY_BUFFER) {
      snprintf(input.buf + input.len, sizeof(input.buf), "%s", buf);
      input.len = min(input.len + len, sizeof(input.buf));
    }
    if (key == SHL_KEY_BACKSPACE) {
      input.len = input.len == 0 ? 0 : input.len - 1;
      input.buf[input.len] = 0;
    }
    if (key == SHL_KEY_CANCEL) {
      exit(0);
    }
  }
  return true;
#undef min
}

void handle_draw(uint32_t w, uint32_t h, ctx_t **ctx) {
  if (ctx_get(w, h, ctx) != OK)
    return;

  ui_start_frame(*ctx, 0x0000000);

  double inputw = config->font.size * 40;
  double inputh = config->font.size * 4;
  double inputx = (double)w / 2 - inputw / 2;
  double inputy = (double)h / 3;

  ui_rect(inputx, inputy, inputw, inputh, 0x000000ff);

  ui_txt_t opts = {
      .font = text,
      .size = config->font.size * 1.75,
      .align = UI_TXT_ALIGN_LEFT,
      .color = 0xffffffff,
      .weight = UI_TXT_WEIGHT_NORMAL,
  };

  ui_txt_bounds_t bounds;
  ui_txt_init(opts, input.buf, &bounds);

  double txtx = inputx + config->font.size;
  double txty = inputy + inputh / 2 + (bounds.ascent - bounds.descent) / 2;
  ui_txt_commit(opts, txtx, txty, &bounds, input.buf);

  double cursorx = txtx + bounds.x_advance + (double)config->font.size / 4;
  double cursorh = bounds.ascent + bounds.descent;
  double cursorw = 2;
  double cursory = txty - bounds.ascent;
  ui_rect(cursorx, cursory, cursorw, cursorh, 0xffffffff);
}

bool handle_pointer(shl_ptr_event_t evt, shl_ptr_btn_t btn, double x,
                    double y) {
  (void)evt;
  (void)btn;
  (void)x;
  (void)y;
  return false;
}

static shl_callbacks_t callbacks = {
    .draw = handle_draw,
    .key = handle_key,
    .pointer = handle_pointer,
};

int main(int argc, char *const *argv) {
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
  assert(shl && "shl must be non-null");

  ui_font_family(config->font.family, &text);
  return shl_run() == OK ? 0 : 1;
}
