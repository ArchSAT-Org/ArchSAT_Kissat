#ifndef _logfile_h_INCLUDED
#define _logfile_h_INCLUDED

#include <stdbool.h>
#include <stdint.h>

void kissat_log_problem_size (unsigned variables, uint64_t clauses);

void kissat_log_count (const char *name, uint64_t value);
void kissat_log_value (const char *name, double value);

void kissat_log_removed_variable (unsigned eidx);

void kissat_log_decided_variable (unsigned eidx, unsigned level);

// Counted toward the average decision level on the next time-window line.
void kissat_log_decision_level (unsigned level);

// False when the matching flag in 'scripts/logging/build-and-test-logger.sh'
// is 0, so the caller can skip work that would only feed the log.
bool kissat_logging_decisions (void);
bool kissat_logging_clauses (void);

// Clauses are logged literal by literal, since they can be arbitrarily long.
void kissat_begin_removed_clause (void);
void kissat_log_removed_clause_literal (int elit);
void kissat_end_removed_clause (void);

// Literals are removed from a clause at a time and the sites which shrink a
// clause have already overwritten it, so only the number of them is known.
void kissat_log_removed_literals (unsigned removed);

struct kissat;

// Real-time summaries. 'kissat_log_reset' starts the clock, 'kissat_log_due'
// says whether a window has closed, and 'kissat_log_tick' writes one.
void kissat_log_reset (double now);
bool kissat_log_due (double now, bool force);
void kissat_log_window (double seconds, uint64_t decisions, uint64_t conflicts,
                        uint64_t propagations, uint64_t removed_variables,
                        uint64_t removed_clauses, uint64_t removed_literals,
                        double bcp, double inprocessing);
void kissat_log_tick (struct kissat *solver, bool force);

// Cheap enough to call from the inner loops: it only reads the clock every
// so often, and writes a line even less often than that.
static inline void kissat_log_tick_sampled (struct kissat *solver) {
  static unsigned calls;
  if (++calls & 63)
    return;
  kissat_log_tick (solver, false);
}

#endif
