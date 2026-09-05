#!/bin/bash

count=$#
sum=0

for arg in "$@"; do
    sum=$((sum + arg))
done

result=$((sum / count))

echo "Количество аргументов: $count"
echo "Среднее арифметическое: $result"

