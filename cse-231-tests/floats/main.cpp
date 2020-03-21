int g1;
int g2;
void bug1() {
  int *p = &g1;
  g2 = *p;
}


float y;
void unary20() {
    y = 1.0;
    y++;
}
