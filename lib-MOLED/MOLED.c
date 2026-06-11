#include "MOLED.h"
#include "MOLED_Font5x7.h"
#include "main.h"

/* 这里直接调用 OLED.c 里的底层页写接口，保持 MOLED 与旧字库解耦。 */
extern void OLED_Init(void);
extern void OLED_Clear(void);
extern void OLED_SetCursor(uint8_t Y, uint8_t X);
extern void OLED_WriteData(uint8_t Data);

#define MOLED_MAX_LINES              4U
#define MOLED_MAX_COLUMNS            16U
#define MOLED_SCROLL_BUFFER_SIZE     160U
#define MOLED_SCROLL_GAP_COLUMNS     16U
#define MOLED_SCROLL_INTERVAL_MS     180U

/* 记录当前控制台光标位置，按 1-based 行列坐标使用。 */
static uint8_t s_line = 1U;
static uint8_t s_column = 1U;

/* 滚动文本状态：只维护一条活动滚动线，符合“这一行一直显示”的需求。 */
static uint8_t s_scrollEnabled = 0U;
static uint8_t s_scrollLine = 1U;
static uint16_t s_scrollLength = 0U;
static uint16_t s_scrollOffset = 0U;
static uint32_t s_scrollLastTick = 0U;
static uint32_t s_scrollIntervalMs = MOLED_SCROLL_INTERVAL_MS;
static char s_scrollText[MOLED_SCROLL_BUFFER_SIZE];

/* 回到左上角，供初始化和清屏后复位光标。 */
static void MOLED_ResetCursor(void)
{
  s_line = 1U;
  s_column = 1U;
}

/* 当前实现是“自动换行”，不是横向滚动；写满最后一行后直接清屏。 */
static void MOLED_NextLine(void)
{
  if (s_line < MOLED_MAX_LINES)
  {
    s_line++;
    s_column = 1U;
  }
  else
  {
    OLED_Clear();
    MOLED_ResetCursor();
  }
}

/* 字库只覆盖可见 ASCII，超出范围的字符统一显示为 '?'. */
static char MOLED_NormalizeChar(char Char)
{
  if ((Char < ' ') || (Char > '~'))
  {
    return '?';
  }

  return Char;
}

/* 将小写字母转成大写，减少字体占用，也让命令显示更统一。 */
static char MOLED_ToDisplayChar(char Char)
{
  if ((Char >= 'a') && (Char <= 'z'))
  {
    Char = (char)(Char - ('a' - 'A'));
  }

  return MOLED_NormalizeChar(Char);
}

static const uint8_t *MOLED_GetIconGlyph(MOLED_IconTypeDef Icon)
{
  switch (Icon)
  {
    case MOLED_ICON_SMILE:
      return MOLED_ICON_SMILE_ROWS;
    case MOLED_ICON_HEART:
      return MOLED_ICON_HEART_ROWS;
    case MOLED_ICON_CHECK:
      return MOLED_ICON_CHECK_ROWS;
    case MOLED_ICON_ARROW_RIGHT:
    default:
      return MOLED_ICON_WARN_ROWS;
  }
}

static void MOLED_DrawGlyph(uint8_t Line, uint8_t Column, const uint8_t *Glyph)
{
  uint8_t top[8] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
  uint8_t bottom[8] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
  uint8_t row;
  uint8_t col;

  for (row = 0U; row < 7U; row++)
  {
    for (col = 0U; col < 5U; col++)
    {
      if ((Glyph[row] & (uint8_t)(1U << (4U - col))) != 0U)
      {
        uint8_t drawX = (uint8_t)(1U + col);
        uint8_t drawY = (uint8_t)(1U + (row * 2U));

        if (drawY < 8U)
        {
          top[drawX] |= (uint8_t)(1U << drawY);
        }
        else
        {
          bottom[drawX] |= (uint8_t)(1U << (drawY - 8U));
        }

        if ((drawY + 1U) < 8U)
        {
          top[drawX] |= (uint8_t)(1U << (drawY + 1U));
        }
        else
        {
          bottom[drawX] |= (uint8_t)(1U << ((drawY + 1U) - 8U));
        }
      }
    }
  }

  OLED_SetCursor((uint8_t)((Line - 1U) * 2U), (uint8_t)((Column - 1U) * 8U));
  for (col = 0U; col < 8U; col++)
  {
    OLED_WriteData(top[col]);
  }

  OLED_SetCursor((uint8_t)(((Line - 1U) * 2U) + 1U), (uint8_t)((Column - 1U) * 8U));
  for (col = 0U; col < 8U; col++)
  {
    OLED_WriteData(bottom[col]);
  }
}

static void MOLED_DrawCharAt(uint8_t Line, uint8_t Column, char Char)
{
  MOLED_DrawGlyph(Line, Column, MOLED_FindGlyphRows(MOLED_ToDisplayChar(Char)));
}

static void MOLED_DrawIconAt(uint8_t Line, uint8_t Column, MOLED_IconTypeDef Icon)
{
  MOLED_DrawGlyph(Line, Column, MOLED_GetIconGlyph(Icon));
}

static void MOLED_RenderStaticString(uint8_t Line, uint8_t Column, const char *String)
{
  uint8_t i = 0U;

  while ((String != 0) && (String[i] != '\0'))
  {
    if (Column > MOLED_MAX_COLUMNS)
    {
      break;
    }

    MOLED_DrawCharAt(Line, Column, String[i]);
    Column++;
    i++;
  }
}

static char MOLED_GetScrollChar(uint16_t Index)
{
  if (Index < s_scrollLength)
  {
    return MOLED_ToDisplayChar(s_scrollText[Index]);
  }

  return ' ';
}

static void MOLED_RenderScrollLine(void)
{
  uint16_t effectiveLength;
  uint8_t column;

  if (s_scrollLength == 0U)
  {
    for (column = 1U; column <= MOLED_MAX_COLUMNS; column++)
    {
      MOLED_DrawCharAt(s_scrollLine, column, ' ');
    }
    return;
  }

  effectiveLength = (uint16_t)(s_scrollLength + MOLED_SCROLL_GAP_COLUMNS);
  if (effectiveLength == 0U)
  {
    effectiveLength = 1U;
  }

  for (column = 1U; column <= MOLED_MAX_COLUMNS; column++)
  {
    uint16_t index = (uint16_t)((s_scrollOffset + (uint16_t)(column - 1U)) % effectiveLength);
    MOLED_DrawCharAt(s_scrollLine, column, MOLED_GetScrollChar(index));
  }
}

/* 初始化底层 OLED，并把控制台光标放回起点。 */
void MOLED_Init(void)
{
  OLED_Init();
  MOLED_ResetCursor();
  s_scrollEnabled = 0U;
  s_scrollLength = 0U;
  s_scrollOffset = 0U;
  s_scrollLastTick = HAL_GetTick();
}

/* 清屏后同步复位控制台光标。 */
void MOLED_Clear(void)
{
  OLED_Clear();
  MOLED_ResetCursor();
  s_scrollEnabled = 0U;
  s_scrollLength = 0U;
  s_scrollOffset = 0U;
}

/* 手动设置控制台输出位置，超出边界会被钳制。 */
void MOLED_SetCursor(uint8_t Line, uint8_t Column)
{
  if (Line < 1U)
  {
    Line = 1U;
  }
  else if (Line > MOLED_MAX_LINES)
  {
    Line = MOLED_MAX_LINES;
  }

  if (Column < 1U)
  {
    Column = 1U;
  }
  else if (Column > MOLED_MAX_COLUMNS)
  {
    Column = MOLED_MAX_COLUMNS;
  }

  s_line = Line;
  s_column = Column;
}

/* 输出单个字符，并维护控制台风格的游标推进逻辑。 */
void MOLED_PutChar(char Char)
{
  if (Char == '\r')
  {
    s_column = 1U;
    return;
  }

  if (Char == '\n')
  {
    MOLED_NextLine();
    return;
  }

  if (Char == '\t')
  {
    uint8_t spaces = (uint8_t)(4U - ((s_column - 1U) % 4U));
    while (spaces-- > 0U)
    {
      MOLED_PutChar(' ');
    }
    return;
  }

  MOLED_DrawCharAt(s_line, s_column, Char);

  if (s_column < MOLED_MAX_COLUMNS)
  {
    s_column++;
  }
  else
  {
    MOLED_NextLine();
  }
}

/* 输出一个图标位，适合做状态图标或“emoji-like”提示。 */
void MOLED_PutIcon(MOLED_IconTypeDef Icon)
{
  MOLED_DrawIconAt(s_line, s_column, Icon);

  if (s_column < MOLED_MAX_COLUMNS)
  {
    s_column++;
  }
  else
  {
    MOLED_NextLine();
  }
}

/* 逐字符输出字符串，不要求调用者自己传入行列。 */
void MOLED_Write(const char *String)
{
  if (String == 0)
  {
    return;
  }

  while (*String != '\0')
  {
    MOLED_PutChar(*String);
    String++;
  }
}

/* 默认把这一行变成滚动文本，更适合上位机命令显示。 */
void MOLED_WriteLine(const char *String)
{
  MOLED_ScrollText(s_line, String);
}

/* 保存一条滚动文本，并在指定行上持续横向滚动。 */
void MOLED_ScrollText(uint8_t Line, const char *String)
{
  uint16_t i = 0U;

  if (Line < 1U)
  {
    Line = 1U;
  }
  else if (Line > MOLED_MAX_LINES)
  {
    Line = MOLED_MAX_LINES;
  }

  s_scrollLine = Line;
  s_scrollEnabled = 1U;
  s_scrollOffset = 0U;
  s_scrollLastTick = HAL_GetTick();

  if (String == 0)
  {
    s_scrollText[0] = '\0';
    s_scrollLength = 0U;
    MOLED_RenderScrollLine();
    return;
  }

  while ((String[i] != '\0') && (i < (MOLED_SCROLL_BUFFER_SIZE - 1U)))
  {
    s_scrollText[i] = String[i];
    i++;
  }
  s_scrollText[i] = '\0';
  s_scrollLength = i;

  MOLED_RenderScrollLine();
}

/* 需要在主循环里周期调用：到时间后就推进一格并重绘整行。 */
void MOLED_Task(void)
{
  uint32_t now;
  uint16_t effectiveLength;

  if (s_scrollEnabled == 0U)
  {
    return;
  }

  now = HAL_GetTick();
  if ((now - s_scrollLastTick) < s_scrollIntervalMs)
  {
    return;
  }

  s_scrollLastTick = now;
  effectiveLength = (uint16_t)(s_scrollLength + MOLED_SCROLL_GAP_COLUMNS);
  if (effectiveLength == 0U)
  {
    effectiveLength = 1U;
  }

  s_scrollOffset++;
  if (s_scrollOffset >= effectiveLength)
  {
    s_scrollOffset = 0U;
  }

  MOLED_RenderScrollLine();
}