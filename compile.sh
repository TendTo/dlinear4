#!/bin/bash
bazel build //dreal --//:enable-soplex=true --compilation_mode=dbg -s