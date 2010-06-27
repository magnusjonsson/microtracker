int util_wrap(int line, int modulo) {
  while(line < 0)
    line += modulo;
  while(line >= modulo)
    line -= modulo;
  return line;
}

