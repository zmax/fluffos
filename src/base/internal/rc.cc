/*
 * rc.c
 * description: runtime configuration for lpmud
 * author: erikkay@mit.edu
 * last modified: July 4, 1994 [robo]
 * Mar 26, 1995: edited heavily by Beek
 * Aug 29, 1998: modified by Gorta
 */

#include "base/internal/rc.h"

#include <cstring>  // for strlen
#include <deque>
#include <fstream>
#include <sstream>
#include <stdlib.h>  // for exit
#include <string>

#include "base/internal/stralloc.h"
#include "base/internal/external_port.h"

char *config_str[NUM_CONFIG_STRS];
int config_int[NUM_CONFIG_INTS];
char *external_cmd[g_num_external_cmds];

namespace {

const int kMaxConfigLineLength = 120;
std::deque<std::string> config_lines;

void config_init() {
  int i;

  for (i = 0; i < NUM_CONFIG_INTS; i++) {
    config_int[i] = 0;
  }
  for (i = 0; i < NUM_CONFIG_STRS; i++) {
    config_str[i] = nullptr;
  }
}

/*
 * If the required flag is 0, it will only give a warning if the line is
 * missing from the config file.  Otherwise, it will give an error and exit
 * if the line isn't there.
 */

/* required:
 1  : Must have
 0  : optional
 -1 : warn if missing
 -2 : warn if found.
 */
const static int kMustHave = 1;
const static int kOptional = 0;
const static int kWarnMissing = -1;
const static int kWarnFound = -2;

bool scan_config_line(const char *fmt, void *dest, int required) {
  /* zero the destination.  It is either a pointer to an int or a char
   buffer, so this will work */
  *(reinterpret_cast<int *>(dest)) = 0;

  bool found = false;
  for (auto line : config_lines) {
    if (sscanf(line.c_str(), fmt, dest) == 1) {
      found = true;
      break;
    }
  }

  std::string line(fmt);
  auto pos = line.find_first_of(':');
  if (pos != std::string::npos) {
    line = line.substr(0, pos);
  }

  if (found) {
    switch (required) {
      case kWarnFound:
        // obsolete
        fprintf(stderr, "*Warning: obsolete line in config file, please delete:\n\t%s\n",
                line.c_str());
        return false;
    }
    return true;
  } else {
    switch (required) {
      case kWarnMissing:
        // optional but warn
        fprintf(stderr, "*Warning: Missing line in config file:\n\t%s\n", line.c_str());
        return false;
      case kOptional:
        // optional
        return false;
      case kMustHave:
        // required
        fprintf(stderr, "*Error in config file.  Missing line:\n\t%s\n", line.c_str());
        exit(-1);
    }
  }
  return false;
}

}  // namespace

void read_config(char *filename) {
  /* needed for string_copy() below */
  CONFIG_INT(__MAX_STRING_LENGTH__) = 128;

  config_init();

  std::ifstream f(filename);
  if (f.is_open()) {
    fprintf(stderr, "using config file: %s\n", filename);
  } else {
    fprintf(stderr, "*Error: couldn't find or open config file: '%s'\n", filename);
    exit(-1);
  }
  std::stringstream buffer;
  buffer << f.rdbuf();

  char tmp[kMaxConfigLineLength];
  while (buffer.getline(&tmp[0], sizeof(tmp), '\n')) {
    if (strlen(tmp) == kMaxConfigLineLength - 1) {
      fprintf(stderr, "*Warning: possible truncated config line: %s", tmp);
    }
    if (strlen(tmp) == 0 || strcmp(tmp, "") == 0) {
      continue;
    }
    // Deal with possible CRLF
    if (strlen(tmp) > 2 && tmp[strlen(tmp) - 2] == '\r') {
      tmp[strlen(tmp) - 2] = '\0';
    }
    config_lines.push_back(std::string(tmp));
  }

  {
    scan_config_line("global include file : %[^\n]", tmp, 0);
    char *p;
    p = CONFIG_STR(__GLOBAL_INCLUDE_FILE__) = alloc_cstring(tmp, "config file: gif");

    /* check if the global include file is quoted */
    if (*p && *p != '"' && *p != '<') {
      char *ptr;

      fprintf(stderr,
              "Missing '\"' or '<' around global include file name; adding "
              "quotes.\n");
      for (ptr = p; *ptr; ptr++) {
        ;
      }
      ptr[2] = 0;
      ptr[1] = '"';
      while (ptr > p) {
        *ptr = ptr[-1];
        ptr--;
      }
      *p = '"';
    }
  }

  scan_config_line("name : %[^\n]", tmp, 1);
  CONFIG_STR(__MUD_NAME__) = alloc_cstring(tmp, "config file: mn");

  scan_config_line("mudlib directory : %[^\n]", tmp, 1);
  CONFIG_STR(__MUD_LIB_DIR__) = alloc_cstring(tmp, "config file: mld");

  scan_config_line("binary directory : %[^\n]", tmp, 1);
  CONFIG_STR(__BIN_DIR__) = alloc_cstring(tmp, "config file: bd");

  scan_config_line("log directory : %[^\n]", tmp, 1);
  CONFIG_STR(__LOG_DIR__) = alloc_cstring(tmp, "config file: ld");

  scan_config_line("include directories : %[^\n]", tmp, 1);
  CONFIG_STR(__INCLUDE_DIRS__) = alloc_cstring(tmp, "config file: id");

  scan_config_line("master file : %[^\n]", tmp, 1);
  CONFIG_STR(__MASTER_FILE__) = alloc_cstring(tmp, "config file: mf");

  scan_config_line("simulated efun file : %[^\n]", tmp, 0);
  CONFIG_STR(__SIMUL_EFUN_FILE__) = alloc_cstring(tmp, "config file: sef");

  scan_config_line("swap file : %[^\n]", tmp, 1);
  CONFIG_STR(__SWAP_FILE__) = alloc_cstring(tmp, "config file: sf");

  scan_config_line("debug log file : %[^\n]", tmp, -1);
  CONFIG_STR(__DEBUG_LOG_FILE__) = alloc_cstring(tmp, "config file: dlf");

  scan_config_line("default error message : %[^\n]", tmp, 0);
  CONFIG_STR(__DEFAULT_ERROR_MESSAGE__) = alloc_cstring(tmp, "config file: dem");

  {
    scan_config_line("default fail message : %[^\n]", tmp, 0);
    if (strlen(tmp) == 0) {
      strcpy(tmp, "What?\n");
    }
    if (strlen(tmp) <= kMaxConfigLineLength - 2) {
      strcat(tmp, "\n");
    }
    CONFIG_STR(__DEFAULT_FAIL_MESSAGE__) = alloc_cstring(tmp, "config file: dfm");
  }

  scan_config_line("mud ip : %[^\n]", tmp, 0);
  CONFIG_STR(__MUD_IP__) = alloc_cstring(tmp, "config file: mi");

  scan_config_line("time to clean up : %d\n", &CONFIG_INT(__TIME_TO_CLEAN_UP__), 1);
  scan_config_line("time to reset : %d\n", &CONFIG_INT(__TIME_TO_RESET__), 1);
  scan_config_line("time to swap : %d\n", &CONFIG_INT(__TIME_TO_SWAP__), 1);

// TODO(sunyc): process these.
#if 0
  /*
   * not currently used...see options.h
   */
  scan_config_line("evaluator stack size : %d\n",
      &CONFIG_INT(__EVALUATOR_STACK_SIZE__), 0);
  scan_config_line("maximum local variables : %d\n",
      &CONFIG_INT(__MAX_LOCAL_VARIABLES__), 0);
  scan_config_line("maximum call depth : %d\n",
      &CONFIG_INT(__MAX_CALL_DEPTH__), 0);
  scan_config_line("living hash table size : %d\n",
      &CONFIG_INT(__LIVING_HASH_TABLE_SIZE__), 0);
#endif

  scan_config_line("inherit chain size : %d\n", &CONFIG_INT(__INHERIT_CHAIN_SIZE__), 1);
  scan_config_line("maximum evaluation cost : %d\n", &CONFIG_INT(__MAX_EVAL_COST__), 1);

  scan_config_line("maximum array size : %d\n", &CONFIG_INT(__MAX_ARRAY_SIZE__), 1);
#ifndef NO_BUFFER_TYPE
  scan_config_line("maximum buffer size : %d\n", &CONFIG_INT(__MAX_BUFFER_SIZE__), 1);
#endif
  scan_config_line("maximum mapping size : %d\n", &CONFIG_INT(__MAX_MAPPING_SIZE__), 1);
  scan_config_line("maximum string length : %d\n", &CONFIG_INT(__MAX_STRING_LENGTH__), 1);
  scan_config_line("maximum bits in a bitfield : %d\n", &CONFIG_INT(__MAX_BITFIELD_BITS__), 1);

  scan_config_line("maximum byte transfer : %d\n", &CONFIG_INT(__MAX_BYTE_TRANSFER__), 1);
  scan_config_line("maximum read file size : %d\n", &CONFIG_INT(__MAX_READ_FILE_SIZE__), 1);

  scan_config_line("hash table size : %d\n", &CONFIG_INT(__SHARED_STRING_HASH_TABLE_SIZE__), 1);
  scan_config_line("object table size : %d\n", &CONFIG_INT(__OBJECT_HASH_TABLE_SIZE__), 1);

  {
    int i, port, port_start = 0;
    if (scan_config_line("port number : %d\n", &CONFIG_INT(__MUD_PORT__), 0)) {
      external_port[0].port = CONFIG_INT(__MUD_PORT__);
      external_port[0].kind = PORT_TELNET;
      port_start = 1;
    }

    /* check for ports */
    if (port_start == 1) {
      if (scan_config_line("external_port_1 : %[^\n]", tmp, 0)) {
        int port = CONFIG_INT(__MUD_PORT__);
        fprintf(stderr,
                "Warning: external_port_1 already defined to be 'telnet %i' by "
                "the line\n    'port number : %i'; ignoring the line "
                "'external_port_1 : %s'\n",
                port, port, tmp);
      }
    }
    for (i = port_start; i < 5; i++) {
      external_port[i].kind = 0;
      external_port[i].fd = -1;

      char kind[kMaxConfigLineLength];
      sprintf(kind, "external_port_%i : %%[^\n]", i + 1);
      if (scan_config_line(kind, tmp, 0)) {
        if (sscanf(tmp, "%s %d", kind, &port) == 2) {
          external_port[i].port = port;
          if (!strcmp(kind, "telnet")) {
            external_port[i].kind = PORT_TELNET;
          } else if (!strcmp(kind, "binary")) {
#ifdef NO_BUFFER_TYPE
            fprintf(stderr, "binary ports unavailable with NO_BUFFER_TYPE defined.\n");
            exit(-1);
#endif
            external_port[i].kind = PORT_BINARY;
          } else if (!strcmp(kind, "ascii")) {
            external_port[i].kind = PORT_ASCII;
          } else if (!strcmp(kind, "MUD")) {
            external_port[i].kind = PORT_MUD;
          } else if (!strcmp(kind, "websocket")) {
            external_port[i].kind = PORT_WEBSOCKET;
          } else {
            fprintf(stderr, "Unknown kind of external port: %s\n", kind);
            exit(-1);
          }
        } else {
          fprintf(stderr, "Syntax error in port specification\n");
          exit(-1);
        }
      }
    }
  }
#ifdef PACKAGE_EXTERNAL
  /* check for commands */
  {
    char kind[kMaxConfigLineLength];

    for (int i = 0; i < g_num_external_cmds; i++) {
      sprintf(kind, "external_cmd_%i : %%[^\n]", i + 1);
      if (scan_config_line(kind, tmp, 0)) {
        external_cmd[i] = alloc_cstring(tmp, "external cmd");
      } else {
        external_cmd[i] = 0;
      }
    }
  }
#endif

  if (!scan_config_line("gametick msec : %d\n", &CONFIG_INT(__RC_GAMETICK_MSEC__), -1)) {
    CONFIG_INT(__RC_GAMETICK_MSEC__) = 1000;  // default to 1s.
  }
  if (!scan_config_line("heartbeat interval msec : %d\n", &CONFIG_INT(__RC_HEARTBEAT_INTERVAL_MSEC__),
                        -1)) {
    // default to match gametick.
    CONFIG_INT(__RC_HEARTBEAT_INTERVAL_MSEC__) = CONFIG_INT(__RC_GAMETICK_MSEC__);
  }

  /*
   * from options.h
   */
  CONFIG_INT(__EVALUATOR_STACK_SIZE__) = CFG_EVALUATOR_STACK_SIZE;
  CONFIG_INT(__MAX_LOCAL_VARIABLES__) = CFG_MAX_LOCAL_VARIABLES;
  CONFIG_INT(__MAX_CALL_DEPTH__) = CFG_MAX_CALL_DEPTH;
  CONFIG_INT(__LIVING_HASH_TABLE_SIZE__) = CFG_LIVING_HASH_SIZE;

  if (!scan_config_line("sane explode string : %d\n", &CONFIG_INT(__RC_SANE_EXPLODE_STRING__), -1)) {
    CONFIG_INT(__RC_SANE_EXPLODE_STRING__) = 1;
  }
  if (!scan_config_line("reversible explode string : %d\n",
                        &CONFIG_INT(__RC_REVERSIBLE_EXPLODE_STRING__), -1)) {
    CONFIG_INT(__RC_REVERSIBLE_EXPLODE_STRING__) = 0;
  }
  if (!scan_config_line("sane sorting : %d\n", &CONFIG_INT(__RC_SANE_SORTING__), -1)) {
    CONFIG_INT(__RC_SANE_SORTING__) = 1;
  }
  if (!scan_config_line("warn tab : %d\n", &CONFIG_INT(__RC_WARN_TAB__), -1)) {
    CONFIG_INT(__RC_WARN_TAB__) = 0;
  }
  if (!scan_config_line("wombles : %d\n", &CONFIG_INT(__RC_WOMBLES__), -1)) {
    CONFIG_INT(__RC_WOMBLES__) = 0;
  }
  if (!scan_config_line("call other type check : %d\n", &CONFIG_INT(__RC_CALL_OTHER_TYPE_CHECK__),
                        -1)) {
    CONFIG_INT(__RC_CALL_OTHER_TYPE_CHECK__) = 0;
  }
  if (!scan_config_line("call other warn : %d\n", &CONFIG_INT(__RC_CALL_OTHER_WARN__), -1)) {
    CONFIG_INT(__RC_CALL_OTHER_WARN__) = 0;
  }
  if (!scan_config_line("mudlib error handler : %d\n", &CONFIG_INT(__RC_MUDLIB_ERROR_HANDLER__), -1)) {
    CONFIG_INT(__RC_MUDLIB_ERROR_HANDLER__) = 1;
  }
  if (!scan_config_line("no resets : %d\n", &CONFIG_INT(__RC_NO_RESETS__), -1)) {
    CONFIG_INT(__RC_NO_RESETS__) = 0;
  }
  if (!scan_config_line("lazy resets : %d\n", &CONFIG_INT(__RC_LAZY_RESETS__), -1)) {
    CONFIG_INT(__RC_LAZY_RESETS__) = 0;
  }
  if (!scan_config_line("randomized resets : %d\n", &CONFIG_INT(__RC_RANDOMIZED_RESETS__), -1)) {
    CONFIG_INT(__RC_RANDOMIZED_RESETS__) = 1;
  }
  if (!scan_config_line("no ansi : %d\n", &CONFIG_INT(__RC_NO_ANSI__), -1)) {
    CONFIG_INT(__RC_NO_ANSI__) = 1;
  }
  if (!scan_config_line("strip before process input : %d\n",
                        &CONFIG_INT(__RC_STRIP_BEFORE_PROCESS_INPUT__), -1)) {
    CONFIG_INT(__RC_STRIP_BEFORE_PROCESS_INPUT__) = 1;
  }
  if (!scan_config_line("this_player in call_out : %d\n", &CONFIG_INT(__RC_THIS_PLAYER_IN_CALL_OUT__),
                        -1)) {
    CONFIG_INT(__RC_THIS_PLAYER_IN_CALL_OUT__) = 1;
  }
  if (!scan_config_line("trace : %d\n", &CONFIG_INT(__RC_TRACE__), -1)) {
    CONFIG_INT(__RC_TRACE__) = 1;
  }
  if (!scan_config_line("trace code : %d\n", &CONFIG_INT(__RC_TRACE_CODE__), -1)) {
    CONFIG_INT(__RC_TRACE_CODE__) = 0;
  }
  if (!scan_config_line("interactive catch tell : %d\n", &CONFIG_INT(__RC_INTERACTIVE_CATCH_TELL__),
                        -1)) {
    CONFIG_INT(__RC_INTERACTIVE_CATCH_TELL__) = 0;
  }
  if (!scan_config_line("receive snoop : %d\n", &CONFIG_INT(__RC_RECEIVE_SNOOP__), -1)) {
    CONFIG_INT(__RC_RECEIVE_SNOOP__) = 1;
  }
  if (!scan_config_line("snoop shadowed : %d\n", &CONFIG_INT(__RC_SNOOP_SHADOWED__), -1)) {
    CONFIG_INT(__RC_SNOOP_SHADOWED__) = 0;
  }
  if (!scan_config_line("reverse defer : %d\n", &CONFIG_INT(__RC_REVERSE_DEFER__), -1)) {
    CONFIG_INT(__RC_REVERSE_DEFER__) = 0;
  }
  if (!scan_config_line("has console : %d\n", &CONFIG_INT(__RC_HAS_CONSOLE__), -1)) {
    CONFIG_INT(__RC_HAS_CONSOLE__) = 1;
  }
  if (!scan_config_line("noninteractive stderr write : %d\n",
                        &CONFIG_INT(__RC_NONINTERACTIVE_STDERR_WRITE__), -1)) {
    CONFIG_INT(__RC_NONINTERACTIVE_STDERR_WRITE__) = 0;
  }
  if (!scan_config_line("trap crashes : %d\n", &CONFIG_INT(__RC_TRAP_CRASHES__), -1)) {
    CONFIG_INT(__RC_TRAP_CRASHES__) = 1;
  }
  if (!scan_config_line("old type behavior : %d\n", &CONFIG_INT(__RC_OLD_TYPE_BEHAVIOR__), -1)) {
    CONFIG_INT(__RC_OLD_TYPE_BEHAVIOR__) = 0;
  }
  if (!scan_config_line("old range behavior : %d\n", &CONFIG_INT(__RC_OLD_RANGE_BEHAVIOR__), -1)) {
    CONFIG_INT(__RC_OLD_RANGE_BEHAVIOR__) = 0;
  }
  if (!scan_config_line("warn old range behavior : %d\n", &CONFIG_INT(__RC_WARN_OLD_RANGE_BEHAVIOR__),
                        -1)) {
    CONFIG_INT(__RC_WARN_OLD_RANGE_BEHAVIOR__) = 1;
  }
  if (!scan_config_line("suppress argument warnings : %d\n",
                        &CONFIG_INT(__RC_SUPPRESS_ARGUMENT_WARNINGS__), -1)) {
    CONFIG_INT(__RC_SUPPRESS_ARGUMENT_WARNINGS__) = 1;
  }

  if (!scan_config_line("enable_commands call init : %d\n",
                        &CONFIG_INT(__RC_ENABLE_COMMANDS_CALL_INIT__), -1)) {
    CONFIG_INT(__RC_ENABLE_COMMANDS_CALL_INIT__) = 1;
  }

  if (!scan_config_line("sprintf add_justified ignore ANSI colors : %d\n",
                        &CONFIG_INT(__RC_SPRINTF_ADD_JUSTFIED_IGNORE_ANSI_COLORS__), -1)) {
    CONFIG_INT(__RC_SPRINTF_ADD_JUSTFIED_IGNORE_ANSI_COLORS__) = 0;
  }

  if (!scan_config_line("apply cache bits : %d\n",
                        &CONFIG_INT(__RC_APPLY_CACHE_BITS__), -1)) {
    CONFIG_INT(__RC_APPLY_CACHE_BITS__) = 22;
  }

  // Complain about obsolete config lines.
  scan_config_line("address server ip : %[^\n]", tmp, -2);
  scan_config_line("address server port : %d\n", tmp, -2);
  scan_config_line("reserved size : %d\n", tmp, -2);
  scan_config_line("fd6 kind : %[^\n]", tmp, -2);
  scan_config_line("fd6 port : %d\n", tmp, -2);

  // Give all obsolete (thus untouched) config strings a value.
  for (int i = 0; i < NUM_CONFIG_STRS; i++) {
    if (config_str[i] == nullptr) {
      config_str[i] = alloc_cstring("", "rc_obsolete");
    }
  }

  // TODO: get rid of config_lines all together.
  config_lines.clear();
  config_lines.shrink_to_fit();
}
