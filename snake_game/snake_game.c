#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/rand.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

#include "ws2818b.pio.h"
#include "ssd1306.h"

#pragma region BUZZER

#define BUZZER_A 21

const float DIVISOR_CLK_PWM = 16.0;

uint16_t next_sfx;

typedef struct note {
  uint16_t value;
  uint32_t duration_ms;
} note;

#define SONG_SIZE 10

note song[SONG_SIZE] = {{7910, 150}, {8385, 150}, {8881, 150}, {9406, 150},
                        {9962, 150}, {10556, 150}, {11183, 150}, {11846, 150},
                        {12555, 150}, {13301, 300}};

void buzzer_init() {
  uint slice;
  gpio_set_function(BUZZER_A, GPIO_FUNC_PWM);
  slice = pwm_gpio_to_slice_num(BUZZER_A);
  pwm_set_clkdiv(slice, DIVISOR_CLK_PWM);
}

void play_note(uint pin, uint16_t wrap) {
  int slice = pwm_gpio_to_slice_num(pin);
  pwm_set_wrap(slice, wrap);
  pwm_set_gpio_level(pin, wrap / 8);
  pwm_set_enabled(slice, true);
}

void stop_note(uint pin) {
  int slice = pwm_gpio_to_slice_num(pin);
  pwm_set_enabled(slice, false);
}

int64_t song_callback(__unused alarm_id_t, __unused void*) {
  static uint8_t curr_note = 0;
  note n = song[curr_note++];
  if (curr_note == SONG_SIZE) {
    stop_note(BUZZER_A);
    return 0;
  }
  play_note(BUZZER_A, n.value);
  return n.duration_ms * 1000;
}

void death_sound() {
  add_alarm_in_ms(1, song_callback, NULL, false);
}

#pragma endregion

#pragma region GAME_LOGIC

#define NUM_ROWS 5
#define NUM_COLS 5
#define BOARD_SIZE 25

typedef enum GAME_OBJS {EPT, SNK, FRT} game_obj_t;
typedef enum BOARD_DIRS {UP, DOWN, LEFT, RIGHT} board_dir_t;

game_obj_t game_board[NUM_ROWS][NUM_COLS] = {0};

typedef struct segment segment_t;

struct segment {
  uint8_t row;
  uint8_t col;
  segment_t *next;
  segment_t *prev;
};

segment_t snake[BOARD_SIZE] = {0};
segment_t *snake_head;
segment_t *snake_last;
segment_t *free_list;
uint8_t snake_size;
board_dir_t head_dir;

void spawn_fruit();

void game_init() {
  game_board[0][0] = SNK;
  snake[0] = (segment_t){0, 0, NULL, NULL};
  snake_head = snake;
  snake_last = snake;
  snake_size = 1;
  head_dir = UP;
  for (uint8_t i = 1; i < BOARD_SIZE - 1; i++) {
    snake[i].next = &snake[i + 1];
  }
  snake[snake_size - 1].next = NULL;
  free_list = &snake[1];
  spawn_fruit();
}

void spawn_fruit() {
  uint8_t next_fruit;
  game_obj_t *b = &game_board[0][0];
  while (true) {
    next_fruit = get_rand_32() % 25;
    if (*(b + next_fruit) == EPT)
      break;
  }
  *(b + next_fruit) = FRT;
}

game_obj_t *next_pos(segment_t *seg, board_dir_t dir) {
  switch (dir) {
    case UP:
      return seg->row >= NUM_ROWS - 1 ? NULL : &(game_board[++seg->row][seg->col]);
    case DOWN:
      return seg->row <= 0 ? NULL : &(game_board[--seg->row][seg->col]);
    case LEFT:
      return seg->col <= 0 ? NULL : &(game_board[seg->row][--seg->col]);
    case RIGHT:
      return seg->col >= NUM_COLS - 1 ? NULL : &(game_board[seg->row][++seg->col]);
  }
}

bool move_snake() {
  bool grow = false;
  segment_t h = *snake_head;
  game_obj_t *next = next_pos(&h, head_dir);
  if (!next || *next == SNK)
    return false;
  if (*next == FRT)
    grow = true;
  segment_t *f = free_list;
  free_list = f->next;
  *f = h;
  f->next = snake_head;
  snake_head->prev = f;
  snake_head = f;
  game_board[h.row][h.col] = SNK;
  if (grow) {
    snake_size++;
    spawn_fruit();
    next_sfx = 8380;
  } else {
    game_board[snake_last->row][snake_last->col] = EPT;
    segment_t *second_last = snake_last->prev;
    snake_last->next = free_list;
    free_list = snake_last;
    snake_last = second_last;
    next_sfx = 29886;
  }
  return true;
}

#pragma endregion

#pragma region LEDS

#define LED_COUNT 25
#define LED_PIN 7

struct pixel_t {
  uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;

npLED_t leds[LED_COUNT];

PIO np_pio;
uint sm;

void npInit(uint pin) {
  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;
  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0) {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true);
  }
  ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
  for (uint i = 0; i < LED_COUNT; ++i) {
    leds[i].R = 0;
    leds[i].G = 0;
    leds[i].B = 0;
  }
}

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}

void npClear() {
  for (uint i = 0; i < LED_COUNT; ++i)
    npSetLED(i, 0, 0, 0);
}

void npWrite() {
  for (uint i = 0; i < LED_COUNT; ++i) {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
  sleep_us(100);
}

void set_leds() {
  npClear();
  for (uint8_t row = 0; row < NUM_ROWS; row++) {
    bool invert = row % 2 == 0;
    for (uint8_t col = 0; col < NUM_COLS; col++) {
      uint8_t led_i;
      if (!invert) {
        led_i = row * 5 + col;
      } else {
        led_i = row * 5 + (NUM_COLS - 1 - col);
      }
      switch (game_board[row][col]) {
      case SNK:
        leds[led_i].G = 100;
        break;
      case FRT:
        leds[led_i].R = 100;
        break;
      default:
        break;
      }
    }
  }
}

void flash_leds() {
  npLED_t leds_aux[LED_COUNT] = {0};
  memcpy(leds_aux, leds, sizeof(leds_aux));
  for (int i = 0; i < 5; i++) {
    memset(leds, 0, sizeof(npLED_t) * LED_COUNT);
    npWrite();
    sleep_ms(100);
    memcpy(leds, leds_aux, sizeof(npLED_t) * LED_COUNT);
    npWrite();
    sleep_ms(100);
  }
}

#pragma endregion

#pragma region JOYSTICK

#define VRY 26
#define VRX 27
#define ERROR_MARGIN 50

volatile board_dir_t last_dir;
repeating_timer_t joystick_timer;

bool joystick_timer_callback(__unused repeating_timer_t *) {
  adc_select_input(1);
  uint x = adc_read();
  adc_select_input(0);
  uint y = adc_read();
  if (y >= 4096 - ERROR_MARGIN) {
    last_dir = UP;
  } else if (y <= ERROR_MARGIN) {
    last_dir = DOWN;
  } else if (x >= 4096 - ERROR_MARGIN) {
    last_dir = RIGHT;
  } else if (x <= ERROR_MARGIN) {
    last_dir = LEFT;
  }
  return true;
}

void joystick_init() {
  adc_init();
  adc_gpio_init(26);
  adc_gpio_init(27);
  add_repeating_timer_ms(100, joystick_timer_callback, NULL, &joystick_timer);
}

#pragma endregion

#pragma region BUTTONS

#define BUTTON_A 5
#define DEBOUNCE_DELAY_MS 120

volatile bool paused = false;
volatile absolute_time_t last_interrupt_time;

void gpio_irq_callback(uint gpio, uint32_t events) {
  absolute_time_t now = get_absolute_time();
  int64_t time_since_last = absolute_time_diff_us(last_interrupt_time, now);
  if (gpio == BUTTON_A && time_since_last < DEBOUNCE_DELAY_MS * 1000)
    return;
  last_interrupt_time = now;
  if (gpio == BUTTON_A && events & GPIO_IRQ_EDGE_FALL) {
    paused = !paused;
    return;
  }
}

void button_init() {
  gpio_init(BUTTON_A);
  gpio_set_dir(BUTTON_A, GPIO_IN);
  gpio_pull_up(BUTTON_A);
  gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, gpio_irq_callback);
  last_interrupt_time = nil_time;
}

#pragma endregion

#pragma region OLED

const uint I2C_SDA_PIN = 14;
const uint I2C_SCL_PIN = 15;

ssd1306_t display;

void oled_init() {
  i2c_init(i2c1, 400000);
  gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA_PIN);
  gpio_pull_up(I2C_SCL_PIN);
  display.external_vcc = false;
  ssd1306_init(&display, 128, 64, 0x3C, i2c1);
}

#pragma endregion

int main() {
  stdio_init_all();
  npInit(LED_PIN);
  npClear();
  joystick_init();
  game_init();
  button_init();
  buzzer_init();
  oled_init();
  while (true) {
    if (paused) {
      while (paused) {
        ssd1306_clear(&display);
        ssd1306_draw_string(&display, 0, 0, 3, "PAUSED");
        ssd1306_show(&display);
        sleep_ms(400);
        ssd1306_clear(&display);
        ssd1306_show(&display);
        sleep_ms(200);
      }
    } else {
      set_leds();
      npWrite();
      char score[2] = {0};
      sprintf(score, "%d", snake_size);
      ssd1306_clear(&display);
      ssd1306_draw_string(&display, 0, 0, 3, "SCORE:");
      ssd1306_draw_string(&display, 0, 32, 3, score);
      ssd1306_show(&display);
      play_note(BUZZER_A, next_sfx);
      head_dir = last_dir;
      if (!move_snake())
        break;
      sleep_ms(200);
      stop_note(BUZZER_A);
      sleep_ms(300);
    }
  }
  stop_note(BUZZER_A);
  death_sound();
  flash_leds();
  sleep_ms(UINT32_MAX);
}
