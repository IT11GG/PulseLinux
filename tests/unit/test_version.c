/* SPDX-License-Identifier: MIT */
/**
 * test_version.c — sanity test for the version string injected at build time.
 *
 * This is intentionally minimal: it proves that:
 *  a) The Meson configure_file() step ran and produced pulse-version.h.
 *  b) PULSE_VERSION and PULSE_VERSION_FULL are non-empty strings.
 *  c) PULSE_VERSION starts with a digit (basic semver sanity).
 *
 * A test runner that exits non-zero is treated as a failure by ninja test.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pulse-version.h"

#define FAIL(msg) \
    do { fprintf(stderr, "FAIL: %s\n", (msg)); return false; } while (0)

static bool test_version_non_empty(void)
{
    if (strlen(PULSE_VERSION) == 0) {
        FAIL("PULSE_VERSION is empty");
    }
    return true;
}

static bool test_version_full_non_empty(void)
{
    if (strlen(PULSE_VERSION_FULL) == 0) {
        FAIL("PULSE_VERSION_FULL is empty");
    }
    return true;
}

static bool test_version_starts_with_digit(void)
{
    if (!isdigit((unsigned char)PULSE_VERSION[0])) {
        FAIL("PULSE_VERSION does not start with a digit — expected semver");
    }
    return true;
}

static bool test_version_full_contains_version(void)
{
    if (strstr(PULSE_VERSION_FULL, PULSE_VERSION) == NULL) {
        FAIL("PULSE_VERSION_FULL does not contain PULSE_VERSION");
    }
    return true;
}

int main(void)
{
    int failures = 0;

    if (!test_version_non_empty())           failures++;
    if (!test_version_full_non_empty())      failures++;
    if (!test_version_starts_with_digit())   failures++;
    if (!test_version_full_contains_version()) failures++;

    if (failures == 0) {
        printf("PASS: version=%s full=%s\n",
               PULSE_VERSION, PULSE_VERSION_FULL);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
