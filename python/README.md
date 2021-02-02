# Python environment

Contains various files to generate plots or analyze results.

## FFT viewer

To evaluate an FFT please ensure the output generation option is enabled.
In **tests** this can be found in tests.hpp as `GENERATE_OUTPUT` and the
generation is determined statically at compile time. Additionally, only
one test case can be printed to the output at a time be sure to check the
value that corresponds to the desired test case.

To output imaginary results instead set `OUTPUT_IMAGINARY` to one. Either
real or imaginary values can be evaluated not both at the same time.

```bash
../cmake-build-debug/tests/testardseq | python evaluate-ffts.py
../cmake-build-debug/tests/testdftseq | python evaluate-ffts.py
```

## Generating csv input files

The implementations use several command line options to facilitate various
features. One of these options is to provide an input file for sample data.

## Merge csv files

The output csv file for stereo wav files will be put into two separate files.
These two files can be merged using `merge-csv.py`, however, it requires
manually editing the names of files in the script.