#!/bin/sh
git ls-files | grep -E '\.(cpp|hpp)$' | grep -v '^external/' | xargs clang-format -i
