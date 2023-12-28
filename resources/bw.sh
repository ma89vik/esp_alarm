#!/bin/bash

for file in *.jpg; do
    convert "$file" -threshold 50% "$file"
done