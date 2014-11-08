native printnum(num);

public main()
{
  new x = 4;
  new y = 8;
  int[][] v = new int[4][8];
  v[2][3] = 9;
  printnum(v[2][3]);
}
