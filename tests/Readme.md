# Tests

This directory contains test scripts which assert Shapetcl's expected behavior. The tests are not necessarily comprehensive. Each `.test.tcl` script may be executed individually, or the `all.tcl` script may be executed to run the entire test suite. Running `make test` in the project root directory invokes `all.tcl`.

The `sample` subdirectory contains shapefiles used by the test scripts (and provided for demonstration purposes). The `tmp` subdirectory is reserved for files that may be written by the test scripts (including copies of the sample shapefiles) and should otherwise be empty.

## Command Tests

These files contain tests that exercise specific Shapetcl [sub]commands. All command options and arguments should be tried, and common error conditions should be triggered.

- `shapefile.test.tcl` tests the main `shapefile` command
- `config.test.tcl` tests the `config` subcommand
- `info.test.tcl` tests the `info` subcommand
- `file.test.tcl` tests the `file` subcommand
- `fields.test.tcl` tests the `fields` subcommand
- `flush.test.tcl` tests the `flush` subcommand
- `coordinates.test.tcl` tests the `coordinates` subcommand
- `attributes.test.tcl` tests the `attributes` subcommand
- `write.test.tcl` tests the `write` subcommand

Note that abbreviated subcommand names are acceptable, so `coordinates` and `attributes` often appear shortened to `coord` and `attr`.

## Miscellaneous Tests

- `exponent.test.tcl` tests "exponential notation" output of floating-point attribute values under certain conditions, a behavior toggled by the `allowAlternateNotation` config option.
- `polygons.test.tcl` checks sample polygon shapefiles for conformity to polygon geometry constraints.
- `samples.test.tcl` checks that basic metadata of sample xy/xym/xyzm shapefiles matches expected values
