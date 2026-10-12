#include "logfile.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LOG_DIRECTORY "log"
#define LOG_PATH LOG_DIRECTORY "/log-output.txt"

// interval between logging windows (in ms)
#define LOG_INTERVAL_MS 50

static FILE *file;
static bool opened;

// sets default for flags
static bool on_decide = true;
static bool on_decide_level = true;
static bool on_remove_variable = true;
static bool on_remove_clause = true;
static bool on_remove_literals = true;
static bool on_total_variables = true;
static bool on_total_clauses = true;
static bool on_time = true;
static bool on_time_decisions = true;
static bool on_time_level = true;
static bool on_time_conflicts = true;
static bool on_time_propagations = true;
static bool on_time_removed_variables = true;
static bool on_time_removed_clauses = true;
static bool on_time_removed_literals = true;
static bool on_time_bcp = true;
static bool on_time_inprocessing = true;
static bool on_summary = true;
static bool on_total_run_time = true;
static bool on_cpu_run_time = true;
static bool on_cpu_percentage = true;
static bool on_solver_time = true;
static bool on_inproc_time = true;
static bool on_inproc_percentage = true;
static bool on_bcp_run_time = true;
static bool on_bcp_percentage = true;
static bool on_removed_clauses_total = true;
static bool on_removed_literals_total = true;

static bool flag_on (const char *name) {
  const char *value = getenv (name);
  return !value || strcmp (value, "0") != 0;
}

// The opposite default: absent or '0' leaves it off.
static bool flag_off (const char *name) {
  const char *value = getenv (name);
  return value && strcmp (value, "0") != 0;
}

static void load_flags (void) {
  static bool loaded;
  if (loaded)
    return;
  loaded = true;
  on_decide = flag_on ("LOG_DECIDE");
  on_decide_level = flag_off ("LOG_DECIDE_LEVEL");
  on_remove_variable = flag_on ("LOG_REMOVE_VARIABLE");
  on_remove_clause = flag_on ("LOG_REMOVE_CLAUSE");
  on_remove_literals = flag_on ("LOG_REMOVE_LITERALS");
  on_total_variables = flag_on ("LOG_TOTAL_VARIABLES");
  on_total_clauses = flag_on ("LOG_TOTAL_CLAUSES");
  on_time = flag_on ("LOG_TIME");
  on_time_decisions = flag_on ("LOG_TIME_DECISIONS");
  on_time_level = flag_on ("LOG_TIME_LEVEL");
  on_time_conflicts = flag_on ("LOG_TIME_CONFLICTS");
  on_time_propagations = flag_on ("LOG_TIME_PROPAGATIONS");
  on_time_removed_variables = flag_on ("LOG_TIME_REMOVED_VARIABLES");
  on_time_removed_clauses = flag_on ("LOG_TIME_REMOVED_CLAUSES");
  on_time_removed_literals = flag_on ("LOG_TIME_REMOVED_LITERALS");
  on_time_bcp = flag_on ("LOG_TIME_BCP");
  on_time_inprocessing = flag_on ("LOG_TIME_INPROCESSING");
  on_summary = flag_on ("LOG_SUMMARY");
  on_total_run_time = flag_on ("LOG_TOTAL_RUN_TIME");
  on_cpu_run_time = flag_on ("LOG_CPU_RUN_TIME");
  on_cpu_percentage = flag_on ("LOG_CPU_PERCENTAGE");
  on_solver_time = flag_on ("LOG_SOLVER_TIME");
  on_inproc_time = flag_on ("LOG_INPROC_TIME");
  on_inproc_percentage = flag_on ("LOG_INPROC_PERCENTAGE");
  on_bcp_run_time = flag_on ("LOG_BCP_RUN_TIME");
  on_bcp_percentage = flag_on ("LOG_BCP_PERCENTAGE");
  on_removed_clauses_total = flag_on ("LOG_REMOVED_CLAUSES_TOTAL");
  on_removed_literals_total = flag_on ("LOG_REMOVED_LITERALS_TOTAL");
}

bool kissat_logging_decisions (void) {
  load_flags ();
  return on_decide;
}

bool kissat_logging_clauses (void) {
  load_flags ();
  return on_remove_clause;
}

static bool summary_on (const char *name) {
  load_flags ();
  if (!on_summary)
    return false;
  if (!strcmp (name, "Total run time"))
    return on_total_run_time;
  if (!strcmp (name, "CPU run time"))
    return on_cpu_run_time;
  if (!strcmp (name, "CPU percentage"))
    return on_cpu_percentage;
  if (!strcmp (name, "Solver time"))
    return on_solver_time;
  if (!strcmp (name, "Inproc time"))
    return on_inproc_time;
  if (!strcmp (name, "Inproc percentage"))
    return on_inproc_percentage;
  if (!strcmp (name, "BCP run time"))
    return on_bcp_run_time;
  if (!strcmp (name, "BCP percentage"))
    return on_bcp_percentage;
  if (!strcmp (name, "Removed clauses"))
    return on_removed_clauses_total;
  if (!strcmp (name, "Removed literals"))
    return on_removed_literals_total;
  return true;
}

static void close_log_file (void) {
  if (!file)
    return;
  fclose (file);
  file = 0;
}

static FILE *log_file (void) {
  if (opened)
    return file;
  opened = true;
  if (mkdir (LOG_DIRECTORY, 0755) && errno != EEXIST)
    return 0;
  file = fopen (LOG_PATH, "w");
  if (!file)
    return 0;
  // Runs are usually aborted by a time limit before 'atexit' triggers.
  setvbuf (file, 0, _IOLBF, 0);
  atexit (close_log_file);
  return file;
}

// Called while parsing the header, so this ends up on the first lines.
void kissat_log_problem_size (unsigned variables, uint64_t clauses) {
  load_flags ();
  if (!on_total_variables && !on_total_clauses)
    return;
  FILE *out = log_file ();
  if (!out)
    return;
  if (on_total_variables)
    fprintf (out, "Total variables %u\n", variables);
  if (on_total_clauses)
    fprintf (out, "Total clauses %" PRIu64 "\n", clauses);
}

void kissat_log_count (const char *name, uint64_t value) {
  if (!summary_on (name))
    return;
  FILE *out = log_file ();
  if (out)
    fprintf (out, "%s %" PRIu64 "\n", name, value);
}

void kissat_log_value (const char *name, double value) {
  if (!summary_on (name))
    return;
  FILE *out = log_file ();
  if (out)
    fprintf (out, "%s %.3f\n", name, value);
}

void kissat_log_removed_variable (unsigned eidx) {
  load_flags ();
  if (!on_remove_variable)
    return;
  FILE *out = log_file ();
  if (out)
    fprintf (out, "LOG: remove variable %u\n", eidx);
}

void kissat_log_decided_variable (unsigned eidx, unsigned level) {
  load_flags ();
  if (!on_decide)
    return;
  FILE *out = log_file ();
  if (!out)
    return;
  if (on_decide_level)
    fprintf (out, "LOG: decide variable %u level %u\n", eidx, level);
  else
    fprintf (out, "LOG: decide variable %u\n", eidx);
}

void kissat_begin_removed_clause (void) {
  load_flags ();
  if (!on_remove_clause)
    return;
  FILE *out = log_file ();
  if (out)
    fputs ("LOG: remove clause", out);
}

void kissat_log_removed_clause_literal (int elit) {
  load_flags ();
  if (!on_remove_clause)
    return;
  FILE *out = log_file ();
  if (out)
    fprintf (out, " %d", elit);
}

void kissat_end_removed_clause (void) {
  load_flags ();
  if (!on_remove_clause)
    return;
  FILE *out = log_file ();
  if (out)
    fputc ('\n', out);
}

void kissat_log_removed_literals (unsigned removed) {
  load_flags ();
  if (!on_remove_literals)
    return;
  FILE *out = log_file ();
  if (out)
    fprintf (out, "LOG: remove %u literals\n", removed);
}

static bool ticking;
static double next_window;
static uint64_t level_sum;
static uint64_t level_count;
static unsigned level_min;
static unsigned level_max;

void kissat_log_decision_level (unsigned level) {
  if (!level_count || level < level_min)
    level_min = level;
  if (!level_count || level > level_max)
    level_max = level;
  level_sum += level;
  level_count++;
}

static double log_interval (void) { return LOG_INTERVAL_MS / 1e3; }

void kissat_log_reset (double now) {
  ticking = true;
  next_window = now + log_interval ();
  level_sum = 0;
  level_count = 0;
  level_min = 0;
  level_max = 0;
}

bool kissat_log_due (double now, bool force) {
  if (!ticking)
    kissat_log_reset (now);
  if (!force && now < next_window)
    return false;
  next_window = now + log_interval ();
  return true;
}

void kissat_log_window (double seconds, uint64_t decisions, uint64_t conflicts,
                        uint64_t propagations, uint64_t removed_variables,
                        uint64_t removed_clauses, uint64_t removed_literals,
                        double bcp, double inprocessing) {
  load_flags ();
  const double average_level =
      level_count ? (double) level_sum / (double) level_count : 0;
  const unsigned lowest_level = level_count ? level_min : 0;
  const unsigned highest_level = level_count ? level_max : 0;
  level_sum = 0;
  level_count = 0;
  level_min = 0;
  level_max = 0;
  if (!on_time)
    return;
  FILE *out = log_file ();
  if (!out)
    return;
  // Dashes fence the window off from the event lines around it.
  fputs ("----------\n", out);
  fprintf (out, "Time: %.3f\n", seconds);
  if (on_time_decisions)
    fprintf (out, "decisions %" PRIu64 "\n", decisions);
  if (on_time_level)
    fprintf (out, "average-level %.3f\nlowest-level %u\nhighest-level %u\n",
             average_level, lowest_level, highest_level);
  if (on_time_bcp)
    fprintf (out, "bcp %.1f%%\n", bcp);
  if (on_time_inprocessing)
    fprintf (out, "inprocessing %.1f%%\n", inprocessing);
  if (on_time_conflicts)
    fprintf (out, "conflicts %" PRIu64 "\n", conflicts);
  if (on_time_propagations)
    fprintf (out, "propagations %" PRIu64 "\n", propagations);
  if (on_time_removed_variables)
    fprintf (out, "removed-variables %" PRIu64 "\n", removed_variables);
  if (on_time_removed_clauses)
    fprintf (out, "removed-clauses %" PRIu64 "\n", removed_clauses);
  if (on_time_removed_literals)
    fprintf (out, "removed-literals %" PRIu64 "\n", removed_literals);
  fputs ("----------\n", out);
}
