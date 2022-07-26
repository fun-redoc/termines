#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define DEFAULT_MINES_PRCTGE 10
#define COVERED_CELL_CHAR '.'
#define LEN(a) ((a==NULL)?0:(sizeof(a)/sizeof(*a)))
#define MAX(a,b) (a>b?a:b)
#define MIN(a,b) (a<b?a:b)

typedef enum {
  Empty = 0,
  Bomb,
} ECellContent;
char cell_chr[] = {' ', '*'};

typedef enum {
  Covered,
  Open,
  Flag
} ECellState;

typedef struct {
  ECellContent content;
  ECellState state;
  size_t n_mines;
} TCell;

typedef struct 
{
  int r;
  int c;
} TCrsr;


struct Field {
  bool field_generated;
  bool show_all;
  int cell_prcntge;
  size_t rows ;
  size_t cols ;
  TCell *cells;
  TCrsr crsr;
} field;

void init_field(rows, cols) {
  if(field.cells) free(field.cells);
  field.cells = malloc(rows*cols*sizeof(TCell));
  field.rows = rows;
  field.cols = cols;

  for(int i=0; i < rows*cols; i++)
  {
    field.cells[i] = (TCell){Empty,Covered,0};
  }
  field.crsr = (TCrsr){0,0};

  field.show_all = false;
  field.cell_prcntge = DEFAULT_MINES_PRCTGE;
}

void free_field()
{
  if(field.cells) free(field.cells);
  field.cells = NULL;
  field.rows = 0;
  field.cols = 0;
}

TCell *get_cell(size_t row, size_t col)
{
  assert(0 <= row && row < field.rows);
  assert(0 <= col && col < field.cols);
  return &(field.cells[row*field.cols + col]);
}

void calc_field() {
  for(int r=0; r < field.rows;r++)
  {
    for(int c=0; c < field.cols;c++)
    {
      for(int i=MAX(0,r-1); i<=MIN(field.rows-1,r+1); i++)
      {
        for(int j=MAX(0,c-1); j<=MIN(field.cols-1,c+1); j++)
        {
          if(!(i==r && j==c))
          {
            get_cell(r,c)->n_mines += get_cell(i,j)->content == Bomb ? 1:0;
          }
        }
      }
    }
  }
}

void print_field(FILE *f, bool uncover_all)
{
  for(size_t row = 0; row < field.rows; row++)
  {
    for(size_t col = 0; col < field.cols; col++)
    {
      TCell c = *get_cell(row,col);
      fprintf(f,field.crsr.c == col && field.crsr.r == row?"[":" ");
      if(uncover_all)
      {
        assert(c.content >= 0 && c.content < LEN(cell_chr));
        if(c.state == Flag) {
          fprintf(f, "P");
        }
        else
        {
          fprintf(f, "%c", cell_chr[c.content]);
        }
      }
      else
      {
        if(c.state == Covered) 
        {
          fprintf(f, "%c", COVERED_CELL_CHAR);
        }
        else if( c.state == Flag)
        {
          fprintf(f, "P");
        }
        else
        {
          if(c.content == Empty)
          {
            if(get_cell(row,col)->n_mines == 0)
            {
              fprintf(f, " ");
            }
            else
            {
              fprintf(f, "%zu", get_cell(row,col)->n_mines);
            }
          }
          else
          {
            fprintf(f, "%c", cell_chr[c.content]);
          }
        }
      }
      fprintf(f,field.crsr.c == col && field.crsr.r == row?"]":" ");
    }
    fprintf(f, "\n");
  }
}

void uncover(size_t row, size_t col);
void uncover_neighbours(size_t row, size_t col)
{
  TCell cell = *get_cell(row,col);
  assert(cell.n_mines == 0);
  for(size_t r=MAX(0,row-1); r<=MIN(field.rows-1, row+1); r++)
  {
    for(size_t c=MAX(0,col-1); c<=MIN(field.cols-1, col+1); c++)
    {
      if(!(r==row && c==col))
      {
        uncover(r,c);
      }
    }
  }
}

void uncover(size_t row, size_t col)
{
  TCell c = *get_cell(row,col);
  if(c.state == Covered)
  {
    c.state = Open;
    *get_cell(row,col) = c;
    if(c.n_mines == 0)
    {
      // i hope i undestood wikopedia correctly means
      // when i uncover a clean field all neighbouring clean fields will be uncovered
      // uncovering stops on fields that are not clean means have >0 mined neighbours
      uncover_neighbours(row,col);
    }
  }
}

// secure cell makes shure that first click is not mine
void generate_random_field(int percentage_of_bombs, int secure_cell_row, int secure_cell_col)
{
  int number_of_bombs = field.rows*field.cols*percentage_of_bombs/100; 
  while(number_of_bombs>0)
  {
      uint32_t random_idx = arc4random_uniform(field.rows*field.cols);
      if(random_idx != field.cols*secure_cell_row + secure_cell_col && 
         field.cells[random_idx].content == Empty)
         {
            field.cells[random_idx] = (TCell){Bomb,Covered,0};
            number_of_bombs--;
         } 
  }
}

bool is_valid_crsr(TCrsr crsr)
{
  return !(crsr.c <0 || crsr.c >=field.cols) && !(crsr.r<0 || crsr.r >= field.rows);
}
TCrsr crsr_add(TCrsr c1, TCrsr c2)
{
  TCrsr res = c1;
  res.c += c2.c;
  res.r += c2.r;
  return res;
}

void crsr_move(TCrsr c) {
  TCrsr new_crsr = crsr_add(field.crsr,c);
  if(is_valid_crsr(new_crsr)) field.crsr = new_crsr;
}

void crsr_up() {
  crsr_move((TCrsr){-1,0});
}
void crsr_down() {
  crsr_move((TCrsr){1,0});
}
void crsr_left() {
  crsr_move((TCrsr){0,-1});
}
void crsr_right() {
  crsr_move((TCrsr){0,1});
}

void crsr_uncover()
{
  if(!field.field_generated)
  {
    generate_random_field(field.cell_prcntge,field.crsr.r,field.crsr.c);
    calc_field();
    field.field_generated = true;
  }
  uncover(field.crsr.r, field.crsr.c);
}

void crsr_flag()
{
  TCell c = *get_cell(field.crsr.r,field.crsr.c);
  if(c.state != Open)
  {
    if(c.state == Flag)
    {
      c.state = Covered;
    }
    else if(c.state == Covered)
    {
      c.state = Flag;
    }
    else
    {
      assert(false && "this case cannot happen.");
    }
    *get_cell(field.crsr.r,field.crsr.c) = c;
  }
}

void show_all()
{
  field.show_all = !field.show_all;
}

struct {
  char c;
  void (*move_fn)();
} movements_map[] = {{'w', &crsr_up},
                     {'a', &crsr_left},
                     {'s', &crsr_down},
                     {'d', &crsr_right},
                     {' ', &crsr_uncover},
                     {'p', &crsr_flag},
                     {'+', &show_all},
                    };
 
void cls(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

int main(void)
{
  init_field(10,10);

  char c;
  do
  {
    cls();
    print_field(stdout,field.show_all);
    c = fgetc(stdin);
    for(int i=0; i<LEN(movements_map); i++)
    {
      if(movements_map[i].c == c)
      {
        (*movements_map[i].move_fn)();
      }
    }
  } while (c != 'q');
  

  free_field();
  return 0;
}
