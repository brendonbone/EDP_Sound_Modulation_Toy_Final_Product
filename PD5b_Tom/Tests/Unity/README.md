# Unity files

`unity.c`, `unity.h`, and `unity_internals.h` were copied from the Unity version
provided in the ZEIT4230 `Testing_Example` course template.

`unity_fixture.c/.h` is a small fixture compatibility layer for this submission.
It provides the fixture macros used by the course (`TEST_GROUP`, `TEST_SETUP`,
`TEST_TEAR_DOWN`, `TEST`, `TEST_GROUP_RUNNER`, `RUN_TEST_CASE`, and
`RUN_TEST_GROUP`) while relying on the course-provided Unity core.

The DSP code performs no dynamic allocation, so the Unity memory extra is not
required by this test suite.
