#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <locale.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include <math.h>

#define KEY_DOWN 0402
#define KEY_UP 0403
#define KEY_LEFT 0404
#define KEY_RIGHT 0405

struct pos{
  int x;
  int y;
};

struct item_info{
  int num;
  int chance;
  int fluc;
  int front_color;
  int back_color;
  int liquid;
  int cross;
  int get;
  char *symbol;
};

int item_n=2; //When you add new item,please change it
struct item_info item[3];
struct item_info grass;
struct pos position;
int stuff_now=0;
int map[1001][1001]={0};
int max_width=1000;
int max_height=1000;
int min_width=1;
int min_height=1;
int dstart_y=1;
int dstart_x=1;
int row=0;
int col=0;
int stuff[101]={0};
int stuff_node[101]={0};
int max_stuff_node=100;
int max_stuff_vis=3;
int max_r_fluc=10;
int pos_old=0;

int display();
int get_pro();
int main_loop();
int init_all();
int init_digital();
int init_license();
void cleanup();
int GetRandom();
int proc(int code);
int get_row_col();
int random_stuff(int num,int chance,int fluc,int liquid);
int item_init();
int get_stuff(int code);
int right_touch(int x,int y);
int liquid_dfs(int x,int y,int num,int r,int max_r);
int check_x_y(int x,int y);
int check_x_y2(int x,int y,int code);

int GetRandom()
{
  int rnum = 0;
  #if defined _MSC_VER
  #if defined _WIN32_WCE
    CeGenRandom(sizeof(int), (PBYTE)&rnum);
  #else
    HCRYPTPROV hProvider = 0;
    const DWORD dwLength = sizeof(int);
    BYTE pbBuffer[dwLength] = {};
    DWORD result =::CryptAcquireContext(&hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
    assert(result);
    result = ::CryptGenRandom(hProvider, dwLength, pbBuffer);
    rnum = *(int*)pbBuffer;
    assert(result);
    ::CryptReleaseContext(hProvider, 0);
  #endif
  #elif defined __GNUC__
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd != -1) {
      (void) read(fd, (void *) &rnum, sizeof(int));
      (void) close(fd);
    }
  #endif
  return rnum;
}

int check_x_y2(int x,int y,int code)
{
  if ((code==1)&&(map[x][y-1]!=0)) return 1;
  if ((code==2)&&(map[x][y+1]!=0)) return 1;
  if ((code==3)&&(map[x-1][y]!=0)) return 1;
  if ((code==4)&&(map[x+1][y]!=0)) return 1;
  return 0;
}

int check_x_y(int x,int y)
{
  int flag[5]={0};
  int i=0,on_sum=4,check_sum=0;
  if (!(y-1>=min_width)) flag[1]=1;
  if (!(y+1<=max_width)) flag[2]=1;
  if (!(x-1>=min_height)) flag[3]=1;
  if (!(x+1<=max_height)) flag[4]=1;
  for (i=1;i<=4;i++)
    if (flag[i]==0) {on_sum--;check_sum+=check_x_y2(x,y,i);}
  if ((on_sum<4)&&(check_sum==4-on_sum)) return 1;
  return 0;
}

int liquid_dfs(int x,int y,int num,int r,int max_r)
{
  int dirct=0;
  if (r==max_r+1)
     return 0;
  else
  {
    while (1)
    {
      dirct=abs(GetRandom())%4+1;
      if (check_x_y(x,y)==1) break;
      if ((dirct==1)&&(y-1>=min_width)&&(map[x][y-1]==0))
      {
        map[x][y-1]=num;
        liquid_dfs(x,y-1,num,r+1,max_r);
      }

      if ((dirct==2)&&(y+1<=max_width)&&(map[x][y+1]==0))
      {
        map[x][y+1]=num;
        liquid_dfs(x,y+1,num,r+1,max_r);
      }

      if ((dirct==3)&&(x-1>=min_height)&&(map[x-1][y]==0))
      {
        map[x-1][y]=num;
        liquid_dfs(x-1,y,num,r+1,max_r);
      }

      if ((dirct==4)&&(x+1<=max_height)&&(map[x+1][y]==0))
      {
        map[x+1][y]=num;
        liquid_dfs(x+1,y,num,r+1,max_r);
      }
    }
  }
  return 0;
}

int right_touch(int x,int y)
{
  if (map[x][y]>=item[1].num)
  {

  }
  if ((map[x][y]==0)&&(stuff[item[stuff_node[stuff_now]].num]-1>=0)&&(stuff_now>0)&&(stuff_now<=max_stuff_vis)&&(stuff_now<=stuff_node[0]))
  {
    map[x][y]=item[stuff_node[stuff_now]].num;
    stuff[item[stuff_node[stuff_now]].num]--;
    if (stuff[item[stuff_node[stuff_now]].num]==0)
    {
      stuff_node[stuff_now]=0;
      stuff_node[0]--;
      stuff_now=0;
    }
  }
  return 0;
}

int get_stuff(int code)
{
  int i=0,flag=0;
  if (code==1)
  {
    if (position.y-1>=min_width)
    {
      if (map[position.x][position.y-1]>=item[1].num)
      {
        if (item[map[position.x][position.y-1]-item[1].num+1].get==1)
        {
          for (i=1;i<=stuff_node[0];i++)
          {
            if (item[stuff_node[i]].num==map[position.x][position.y-1])
            {
              flag=1;
              stuff[item[stuff_node[i]].num]++;
              map[position.x][position.y-1]=0;
            }
          }
          if ((flag==0)&&(stuff_node[0]+1<=max_stuff_node))
          {
            stuff_node[0]++;
            stuff_node[stuff_node[0]]=map[position.x][position.y-1]-item[1].num+1;
            stuff[map[position.x][position.y-1]]++;
            map[position.x][position.y-1]=0;
          }
        }
        i=0;
      }
    }
  }

  if (code==2)
  {
    if (position.y+1<=max_width)
    {
      if (map[position.x][position.y+1]>=item[1].num)
      {
        if (item[map[position.x][position.y+1]-item[1].num+1].get==1)
        {
          for (i=1;i<=stuff_node[0];i++)
          {
            if (item[stuff_node[i]].num==map[position.x][position.y+1])
            {
              flag=1;
              stuff[item[stuff_node[i]].num]++;
              map[position.x][position.y+1]=0;
            }
          }
          if ((flag==0)&&(stuff_node[0]+1<=max_stuff_node))
          {
            stuff_node[0]++;
            stuff_node[stuff_node[0]]=map[position.x][position.y+1]-item[1].num+1;
            stuff[map[position.x][position.y+1]]++;
            map[position.x][position.y+1]=0;
          }
        }
        i=0;
      }
    }
  }

  if (code==3)
  {
    if (position.x-1>=min_height)
    {
      if (map[position.x-1][position.y]>=item[1].num)
      {
        if (item[map[position.x-1][position.y]-item[1].num+1].get==1)
        {
          for (i=1;i<=stuff_node[0];i++)
          {
            if (item[stuff_node[i]].num==map[position.x-1][position.y])
            {
              flag=1;
              stuff[item[stuff_node[i]].num]++;
              map[position.x-1][position.y]=0;
            }
          }
          if ((flag==0)&&(stuff_node[0]+1<=max_stuff_node))
          {
            stuff_node[0]++;
            stuff_node[stuff_node[0]]=map[position.x-1][position.y]-item[1].num+1;
            stuff[map[position.x-1][position.y]]++;
            map[position.x-1][position.y]=0;
          }
        }
        i=0;
      }
    }
  }

  if (code==4)
  {
    if (position.x+1<=max_height)
    {
      if (map[position.x+1][position.y]>=item[1].num)
      {
        if (item[map[position.x+1][position.y]-item[1].num+1].get==1)
        {
          for (i=1;i<=stuff_node[0];i++)
          {
            if (item[stuff_node[i]].num==map[position.x+1][position.y])
            {
              flag=1;
              stuff[item[stuff_node[i]].num]++;
              map[position.x+1][position.y]=0;
            }
          }
          if ((flag==0)&&(stuff_node[0]+1<=max_stuff_node))
          {
            stuff_node[0]++;
            stuff_node[stuff_node[0]]=map[position.x+1][position.y]-item[1].num+1;
            stuff[map[position.x+1][position.y]]++;
            map[position.x+1][position.y]=0;
          }
        }
        i=0;
      }
    }
  }
  return 0;
}

int item_init()
{
  item[1].num=7;
  item[1].chance=6000;
  item[1].fluc=1000;
  item[1].symbol="#";
  item[1].front_color=COLOR_WHITE;
  item[1].back_color=COLOR_YELLOW;
  item[1].liquid=0;
  item[1].cross=0;//can't
  item[1].get=1;//can

  item[2].num=8;
  item[2].chance=30;
  item[2].fluc=3;
  item[2].symbol="'";
  item[2].front_color=COLOR_WHITE;
  item[2].back_color=COLOR_BLUE;
  item[2].liquid=1;
  item[2].cross=1;//can't
  item[2].get=0;//can
  return 0;
}

int random_stuff(int num,int chance,int fluc,int liquid)
{
  int i=0,ran_x=0,ran_y=0,flag=0;
  flag=0;
  if (liquid==0)
  {
    for (i=1;i<=(chance+(abs(GetRandom())%fluc));i++)
    {
      flag=0;
      while (flag==0)
      {
        ran_x=0;ran_y=0;
        ran_x=abs(GetRandom())%max_height+1;
        ran_y=abs(GetRandom())%max_width+1;
        if ((ran_x!=position.x)&&(ran_y!=position.y)&&(map[ran_x][ran_y]==0))
        {
          flag=1;
          map[ran_x][ran_y]=num;
        }
      }
    }
  }
  flag=0;
  if (liquid==1)
  {
    for (i=1;i<=(chance+(abs(GetRandom())%fluc));i++)
    {
      flag=0;
      while (flag==0)
      {
        ran_x=0;ran_y=0;
        ran_x=abs(GetRandom())%max_height+1;
        ran_y=abs(GetRandom())%max_width+1;
        if ((ran_x!=position.x)&&(ran_y!=position.y)&&(map[ran_x][ran_y]==0))
        {
          flag=1;
          map[ran_x][ran_y]=num;
          liquid_dfs(ran_x,ran_y,num,1,25);
        }
      }
    }
  }
  return 0;
}

int get_row_col()
{
  struct winsize info;
  ioctl(STDIN_FILENO,TIOCGWINSZ,&info);
  /*if ((position.x>(dstart_x+info.ws_row-2))||(position.y>(dstart_y+info.ws_col-1)))
  {
    dstart_x=position.x;
    dstart_y=position.y;
  }*/
  row=info.ws_row;
  col=info.ws_col;
  return 0;
}

int display()
{
  int i=0,j=0,k=0;
  erase();
  refresh();
  for (i=dstart_x;i<=dstart_x+(row-2);i++)
    for (j=dstart_y;j<=dstart_y+(col-1);j++)
    {
      if (map[i][j]==1) printw("▢");
      if (map[i][j]==0) printw(" ");
      for (k=1;k<=item_n;k++)
        if (map[i][j]==item[k].num)
        {
          init_pair(item[k].num,item[k].front_color,item[k].back_color);
          attron(COLOR_PAIR(item[k].num));
          printw("%s",item[k].symbol);
          attroff(COLOR_PAIR(item[k].num));
        }
    }
  refresh();
  move(row-1,0);refresh();
  //strat color(black,white)
  init_pair(1,COLOR_BLACK,COLOR_WHITE);
  attron(COLOR_PAIR(1));

  printw("X:%d Y:%d row:%d col:%d",position.x,position.y,row,col);refresh();
  if (stuff_now>0)
    printw(" N:%s",item[stuff_node[stuff_now]].symbol);
  else printw(" N:Hand");
  printw(" S(%d):",stuff_node[0]);
  if (stuff_node[0]<max_stuff_vis)
  {
    for (i=1;i<=stuff_node[0];i++)
      printw("%d %s ",stuff[item[stuff_node[i]].num],item[stuff_node[i]].symbol);refresh();
  }
  if (stuff_node[0]>=max_stuff_vis)
  {
    for (i=1;i<=max_stuff_vis;i++)
      printw("%d %s ",stuff[item[stuff_node[i]].num],item[stuff_node[i]].symbol);refresh();
  }
  attroff(COLOR_PAIR(1));
  //stop color(black,white)
  return 0;
}

int direction(int code)
{
  int k=0,num=0;
  if (code==1) //left
  {
    num=0;
    for (k=1;k<=item_n;k++)
    if (position.y-1>=min_width)
      if (map[position.x][position.y-1]==item[k].num) num=k;
    if ((position.y-1>=min_width)&&((item[num].cross==1)||(map[position.x][position.y-1]==0)))
    {
      map[position.x][position.y]=pos_old;
      pos_old=map[position.x][position.y-1];
      position.y--;
      if (position.y<dstart_y) dstart_y--;
      map[position.x][position.y]=1;
    }
  }

  if (code==2) //right
  {
    num=0;
    for (k=1;k<=item_n;k++)
    if (position.y+1<=max_width)
      if (map[position.x][position.y+1]==item[k].num) num=k;
    if ((position.y+1<=max_width)&&((item[num].cross==1)||(map[position.x][position.y+1]==0)))
    {
      map[position.x][position.y]=pos_old;
      pos_old=map[position.x][position.y+1];
      position.y++;
      if (position.y>(dstart_y+(col-1))) dstart_y++;
      map[position.x][position.y]=1;
    }
  }

  if (code==3) //up
  {
    num=0;
    for (k=1;k<=item_n;k++)
    if (position.x-1>=min_height)
      if (map[position.x-1][position.y]==item[k].num) num=k;
    if ((position.x-1>=min_height)&&((item[num].cross==1)||(map[position.x-1][position.y]==0)))
    {
      map[position.x][position.y]=pos_old;
      pos_old=map[position.x-1][position.y];
      position.x--;
      if (position.x<dstart_x) dstart_x--;
      map[position.x][position.y]=1;
    }
  }

  if (code==4) //down
  {
    num=0;
    for (k=1;k<=item_n;k++)
    if (position.x+1<=max_height)
      if (map[position.x+1][position.y]==item[k].num) num=k;
    if ((position.x+1<=max_height)&&((item[num].cross==1)||(map[position.x+1][position.y]==0)))
    {
      map[position.x][position.y]=pos_old;
      pos_old=map[position.x+1][position.y];
      position.x++;
      if (position.x>(dstart_x+(row-2))) dstart_x++;
      map[position.x][position.y]=1;
    }
  }
  return 0;
}

int get_pro()
{
  int ch=0;
  halfdelay(1);
  ch=getch();
  //directions
  if (ch==KEY_LEFT) direction(1);
  if (ch==KEY_RIGHT) direction(2);
  if (ch==KEY_UP) direction(3);
  if (ch==KEY_DOWN) direction(4);

  //get stuff
  if (ch=='a') get_stuff(1);
  if (ch=='d') get_stuff(2);
  if (ch=='w') get_stuff(3);
  if (ch=='s') get_stuff(4);

  //right touch the stuff
  if ((ch==1)&&(position.y-1>=min_width)) right_touch(position.x,position.y-1);
  if ((ch==4)&&(position.y+1<=max_width)) right_touch(position.x,position.y+1);
  if ((ch==23)&&(position.x-1>=min_height)) right_touch(position.x-1,position.y);
  if ((ch==19)&&(position.x+1<=max_height)) right_touch(position.x+1,position.y);

  //change the stuff_now
  if ((ch=='0')&&(stuff_node[0]>=0)&&(0<=max_stuff_vis)) stuff_now=0;
  if ((ch=='1')&&(stuff_node[0]>=1)&&(1<=max_stuff_vis)) stuff_now=1;
  if ((ch=='2')&&(stuff_node[0]>=2)&&(2<=max_stuff_vis)) stuff_now=2;
  if ((ch=='3')&&(stuff_node[0]>=3)&&(3<=max_stuff_vis)) stuff_now=3;
  if ((ch=='4')&&(stuff_node[0]>=4)&&(4<=max_stuff_vis)) stuff_now=4;
  if ((ch=='5')&&(stuff_node[0]>=5)&&(5<=max_stuff_vis)) stuff_now=5;
  if ((ch=='6')&&(stuff_node[0]>=6)&&(6<=max_stuff_vis)) stuff_now=6;
  if ((ch=='7')&&(stuff_node[0]>=7)&&(7<=max_stuff_vis)) stuff_now=7;
  if ((ch=='8')&&(stuff_node[0]>=8)&&(8<=max_stuff_vis)) stuff_now=8;
  if ((ch=='9')&&(stuff_node[0]>=9)&&(9<=max_stuff_vis)) stuff_now=9;

  if (ch==27) return 1;
  return 0;
}

int main_loop()
{
  int r=0;
  display();
  while (1)
  {
    r=0;
    r=get_pro();
    get_row_col();
    if (r==1) break;
    display();
  }
  return 0;
}

int init_all()
{
  init_license();
  init_digital();
  return 0;
}

int init_license()
{
  setlocale(LC_ALL, "en_US.UTF-8");
  initscr();
  raw();
  keypad(stdscr, TRUE);
  noecho();
  curs_set(0);
  if (has_colors()==TRUE) start_color();
  printw("Welcome to Craft-L!\n");
  printw("The software is using GPL (http://www.gnu.org/licenses/gpl.txt)\n");
  printw("Press any key to agree and play\n");
  refresh();
  getch();
  return 0;
}

int init_digital()
{
  int i=0;
  item_init();
  position.x=(abs(GetRandom())%900+1);
  position.y=(abs(GetRandom())%900+1);
  //position.x=24;
  //position.y=80;
  pos_old=map[position.x][position.y];
  get_row_col();
  if ((position.x%(row-1))!=0)
    dstart_x=(abs(GetRandom())%(position.x%(row-1))+(position.x-(position.x%(row-1))))+1;
  else dstart_x=position.x;
  if ((position.y%col)!=0)
    dstart_y=(abs(GetRandom())%(position.y%col)+(position.y-(position.y%col)))+1;
  else dstart_y=position.y;

  map[position.x][position.y]=1;
  for (i=1;i<=item_n;i++)
    random_stuff(item[i].num,item[i].chance,item[i].fluc,item[i].liquid);
  return 0;
}

void cleanup()
{
  endwin();
}

int main()
{
  atexit(cleanup);
  init_all();
  main_loop();
  return 0;
}
