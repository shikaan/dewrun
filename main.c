#include "src/array.h"
#include "src/cli.h"
#include "src/config.h"
#include "src/ctx.h"
#include "src/fuzzy.h"
#include "src/log.h"
#include "src/result.h"
#include "src/shell.h"
#include "src/spawn.h"
#include "src/ui.h"
#include <assert.h>
#include <src/color.h>
#include <src/run.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ui_font_t *text = NULL;

static literals_t *results = NULL;
static Buffer(char, 1024) term = {0};
static literals_t *input_list = NULL;
static size_t selected_idx = 0;
static size_t offset = 0;
static config_t *config = NULL;
static size_t cursor = 0;

// TODO: should this be configuration?
enum { MAX_FILTERED = 5 };

static size_t utf8_cursor_retreat(const char *s, size_t i) {
  if (i == 0)
    return 0;

  i--;
  while (i > 0 && ((unsigned char)s[i] & 0xc0) == 0x80)
    i--;
  return i;
}

static size_t utf8_cursor_advance(const char *s, size_t len, size_t i) {
  if (i >= len)
    return len;

  i++;
  while (i < len && ((unsigned char)s[i] & 0xc0) == 0x80)
    i++;
  return i;
}

bool handle_key(shl_kbd_event_t evt, shl_key_t key, char *buf, size_t len) {
  if (evt == SHL_KBD_EVENT_KEYDOWN) {
    switch (key) {
    case SHL_KEY_BUFFER: {
      if (len > ArraySize(term.buf) - 1 - term.len)
        break;

      memmove(term.buf + cursor + len, term.buf + cursor, term.len - cursor);
      memcpy(term.buf + cursor, buf, len);
      term.len += len;
      term.buf[term.len] = 0;
      cursor += len;

      fuz_find(ToSlice(literal_t, term), input_list, &results);
      selected_idx = 0;
      offset = 0;
      break;
    }
    case SHL_KEY_BACKSPACE: {
      if (!cursor)
        break;

      size_t prev = utf8_cursor_retreat(term.buf, cursor);
      memmove(term.buf + prev, term.buf + cursor, term.len - cursor);
      term.len -= cursor - prev;
      term.buf[term.len] = 0;
      cursor = prev;

      fuz_find(ToSlice(literal_t, term), input_list, &results);
      selected_idx = 0;
      offset = 0;
      break;
    }
    case SHL_KEY_DELETE: {
      if (cursor == term.len)
        break;

      size_t next = utf8_cursor_advance(term.buf, term.len, cursor);
      memmove(term.buf + cursor, term.buf + next, term.len - cursor);
      term.len -= next - cursor;
      term.buf[term.len] = 0;

      fuz_find(ToSlice(literal_t, term), input_list, &results);
      selected_idx = 0;
      offset = 0;
      break;
    }
    case SHL_KEY_DOWN:
      if (!results->len)
        break;
      selected_idx += selected_idx < results->len - 1;
      offset += selected_idx != results->len - 1 &&
                selected_idx - offset == MAX_FILTERED - 1;
      break;
    case SHL_KEY_UP:
      selected_idx -= selected_idx > 0;
      offset -= offset > 0 && selected_idx - offset == 0;
      break;
    case SHL_KEY_LEFT:
      cursor = utf8_cursor_retreat(term.buf, cursor);
      break;
    case SHL_KEY_RIGHT:
      cursor = utf8_cursor_advance(term.buf, term.len, cursor);
      break;
    case SHL_KEY_CANCEL:
      exit(0);
    case SHL_KEY_CONFIRM:
      if (results->len == 0)
        break;
      if (spw_launch(results->buf[selected_idx].buf) == OK) {
        exit(0);
      }
      // TODO: error handling
      break;
    case SHL_KEY_UNKNOWN:
    default:
      return false;
    }
  }

  return true;
}

void handle_draw(uint32_t w, uint32_t h, ctx_t **ctx) {
  if (ctx_get(w, h, ctx) != OK)
    return;

  ui_start_frame(*ctx, 0x0000044);

  double inputw = config->font.size * 40;
  double inputh = config->font.size * 4;
  double inputx = (double)w / 2 - inputw / 2;
  double inputy = (double)h / 3;

  ui_rect(inputx, inputy, inputw, inputh, 0x000000ff);

  ui_txt_t opts = {
      .font = text,
      .size = config->font.size * 1.5,
      .align = UI_TXT_ALIGN_LEFT,
      .color = 0xffffffff,
      .weight = UI_TXT_WEIGHT_NORMAL,
  };

  ui_txt_bounds_t bounds;
  ui_txt_init(opts, term.buf, &bounds);

  double resultw = inputw;
  double resulth = inputh;
  double resultx = inputx;
  double resultybase = inputy + inputh;
  if (results) {
    double restxtx = resultx + config->font.size;
    size_t nresults = results->len < MAX_FILTERED ? results->len : 5;
    for (size_t i = 0; i < nresults; i++) {
      double resulty = resultybase + resulth * (double)i;
      double restxty =
          resulty + resulth / 2 + (bounds.ascent - bounds.descent) / 2;

      color_t background = i + offset == selected_idx ? 0x82a2beff : 0x000000ff;
      ui_rect(inputx, resulty, resultw, resulth, background);
      opts.color = 0xffffff99;
      ui_txt(opts, restxtx, restxty, results->buf[i + offset].buf);
    }
  }

  opts.color = 0xffffffff;
  ui_txt_init(opts, term.buf, &bounds);
  double txtx = inputx + config->font.size;
  double txty = inputy + inputh / 2 + (bounds.ascent - bounds.descent) / 2;
  if (term.len) {
    ui_txt_commit(opts, txtx, txty, &bounds, term.buf);
  }

  char backup = term.buf[cursor];
  term.buf[cursor] = 0;
  ui_txt_init(opts, term.buf, &bounds);
  double cursorx = txtx + bounds.x_advance;
  double cursorh = bounds.ascent + bounds.descent;
  double cursorw = 2;
  double cursory = txty - bounds.ascent;
  ui_rect(cursorx, cursory, cursorw, cursorh, 0xffffffff);
  term.buf[cursor] = backup;
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

  // TODO: this list leaks memory. Should be cleaned up before closing.
  run_init(&input_list);
  fuz_find(ToSlice(literal_t, term), input_list, &results);

  return shl_run() == OK ? 0 : 1;
}
