#include <nds.h>
#include <filesystem.h>
#include <cerrno>
#include <cstdio>

#if 0
#define WIDTH   128
#define HEIGHT  96
#define BGTYPE  BgSize_B8_128x128
#define BGSCALE (1 << 7)
#else
#define WIDTH   SCREEN_WIDTH
#define HEIGHT  SCREEN_HEIGHT
#define BGTYPE  BgSize_B8_256x256
#define BGSCALE (1 << 8)
#endif

static const u8 alive_table[256] DTCM_DATA =
{
  0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0,
  0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0,
  1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0,
  1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0,
  1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const u8 dead_table[256] DTCM_DATA =
{
  0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0,
  0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0,
  0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0,
  0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0,
  0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0,
  0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const char* grids[] =
{
  "nitro:/gosper.dat",
  "nitro:/acorn.dat",
  "nitro:/block.dat",
  "nitro:/block2.dat",
  "nitro:/block3.dat",
  "nitro:/diehard.dat",
  "nitro:/pentomino.dat",
  "nitro:/pulsar.dat",
};
#define NUM_GRIDS (sizeof(grids) / sizeof(char*))

static inline size_t coord(size_t x, size_t y)
{
  return y*WIDTH + x;
}

static inline u8 check(size_t x, size_t y, const u8 *now)
{
  return now[coord(x, y)];
}

static void life(const u8* now, u8* next)
{
  for(size_t j = 0; j < HEIGHT; ++j)
  {
    for(size_t i = 0; i < WIDTH; ++i)
    {
      u8 alive = check(i, j, now);
      u8 num_live = 0;

      if(i == 0)
      {
        if(j == 0)
        {
          num_live = check(i  , j+1, now)
              | (check(i+1, j  , now) << 1)
              | (check(i+1, j+1, now) << 2);
        }
        else if(j == HEIGHT-1)
        {
          num_live = check(i  , j-1, now)
              | (check(i+1, j  , now) << 1)
              | (check(i+1, j-1, now) << 2);
        }
        else
        {
          num_live = check(i  , j-1, now)
              | (check(i  , j+1, now) << 1)
              | (check(i+1, j-1, now) << 2)
              | (check(i+1, j  , now) << 3)
              | (check(i+1, j+1, now) << 4);
        }
      }
      else if(i == WIDTH-1)
      {
        if(j == 0)
        {
          num_live = check(i-1, j  , now)
              | (check(i-1, j+1, now) << 1)
              | (check(i  , j+1, now) << 2);
        }
        else if(j == HEIGHT-1)
        {
          num_live = check(i-1, j  , now)
              | (check(i-1, j-1, now) << 1)
              | (check(i  , j-1, now) << 2);
        }
        else
        {
          num_live = check(i-1, j-1, now)
              | (check(i-1, j  , now) << 1)
              | (check(i-1, j+1, now) << 2)
              | (check(i  , j-1, now) << 3)
              | (check(i  , j+1, now) << 4);
        }
      }
      else
      {
        if(j == 0)
        {
          num_live = check(i-1, j  , now)
              | (check(i-1, j+1, now) << 1)
              | (check(i  , j+1, now) << 2)
              | (check(i+1, j+1, now) << 3)
              | (check(i+1, j  , now) << 4);
        }
        else if(j == HEIGHT-1)
        {
          num_live = check(i-1, j  , now)
              | (check(i-1, j-1, now) << 1)
              | (check(i  , j-1, now) << 2)
              | (check(i+1, j-1, now) << 3)
              | (check(i+1, j  , now) << 4);
        }
        else
        {
          num_live = check(i-1, j-1, now)
              | (check(i-1, j  , now) << 1)
              | (check(i-1, j+1, now) << 2)
              | (check(i  , j+1, now) << 3)
              | (check(i+1, j+1, now) << 4)
              | (check(i+1, j  , now) << 5)
              | (check(i+1, j-1, now) << 6)
              | (check(i  , j-1, now) << 7);
        }
      }

      next[coord(i, j)] = (alive) ? alive_table[num_live] : dead_table[num_live];
    }
  }
}

static void loadGen(const char* path, u8* gen)
{
  iprintf("Opening %s:", path);

  FILE *fp = fopen(path, "r");
  if(fp == NULL)
  {
    iprintf("fopen: %s\n", strerror(errno));
    return;
  }

  memset(gen, 0, WIDTH * HEIGHT);

  size_t width, height;
  size_t x_offset, y_offset;

  fiscanf(fp, "%u %u", &width, &height);
  iprintf("%ux%u\n", width, height);

  x_offset = (WIDTH - width) / 2;
  y_offset = (HEIGHT - height) / 2;
  for(size_t j = 0; j < height; ++j)
  {
    for(size_t i = 0; i < width; ++i)
    {
      size_t alive;
      fiscanf(fp, "%u", &alive);
      gen[(j+y_offset)*WIDTH + (i+x_offset)] = alive;
    }
  }

  fclose(fp);
}

static u8 generation[2][WIDTH * HEIGHT];

int main(int argc, char** argv)
{
  u16    *gfx;
  size_t cur_gen = 0;
  int    down;

  videoSetMode(MODE_3_2D);
  videoSetModeSub(MODE_0_2D);
  vramSetPrimaryBanks(VRAM_A_MAIN_BG,
                      VRAM_B_MAIN_SPRITE,
                      VRAM_C_SUB_BG,
                      VRAM_D_SUB_SPRITE);

  consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 2, 0, false, true);
  bgInit(3, BgType_Bmp8, BGTYPE, 0, 0);
  bgSetScale(3, BGSCALE, BGSCALE);
  bgUpdate();
  gfx = bgGetGfxPtr(3);

  if(!nitroFSInit(NULL))
  {
    iprintf("Unsuccessful");
    do
    {
      swiWaitForVBlank();
      down = keysDown();
    } while(!(down & KEY_B));
    return 1;
  }

  iprintf("%s\n", argv[0]);

  dmaFillHalfWords(0, gfx, WIDTH * HEIGHT);
  BG_PALETTE[0] = RGB15( 0,  0,  0);
  BG_PALETTE[1] = RGB15(31, 31, 31);

  loadGen(grids[0], generation[0]);

  while(true)
  {
    DC_FlushAll();
    dmaCopy(generation[cur_gen%2], gfx, WIDTH * HEIGHT);
    swiWaitForVBlank();
    scanKeys();
    down = keysDown();

    life(generation[cur_gen%2], generation[(cur_gen+1)%2]);
    ++cur_gen;

    if(down & KEY_A)
    {
      static size_t choice = 0;
      choice += 1;
      if(choice >= NUM_GRIDS)
        choice = 0;

      loadGen(grids[choice], generation[cur_gen%2]);
    }
  }

  return 0;
}
