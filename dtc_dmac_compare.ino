#include <Arduino.h>
#include "FspTimer.h"
#include "r_dtc.h"
#include "r_dmac.h"

// DTC control variables.
// These are needed by the DTC driver.
dtc_instance_ctrl_t v_dtc_ctrl;
transfer_info_t v_dtc_info;
dtc_extended_cfg_t v_dtc_extend;
transfer_cfg_t v_dtc_cfg;

// DMA control variables.
// These are needed by the DMA driver.
dmac_instance_ctrl_t v_dma_ctrl;
transfer_info_t v_dma_info;
dmac_extended_cfg_t v_dma_extend;
transfer_cfg_t v_dma_cfg;

// Timer object.
// We use this timer to make 1 event every 1 second.
FspTimer v_timer;
uint8_t v_timer_type = 0;
int8_t v_timer_index = -1;

// Source data buffer.
// DTC and DMA will copy this data.
uint16_t v_src[8] = {10, 20, 30, 40, 50, 60, 70, 80};

// Destination buffers.
// v_dtc is filled by DTC.
// v_dma is filled by DMA.
uint16_t v_dtc[8] = {};
uint16_t v_dma[8] = {};

// These flags are used to know what step is done.
bool v_dma_done = false;
bool v_dtc_started = false;
bool v_print_done = false;
bool v_demo_done = false;

// Time and cycle variables.
unsigned long v_start_time = 0;
unsigned long v_dma_time = 0;
unsigned long v_cycle_done_time = 0;
uint32_t v_count = 0;

void v_clear_data() {
  // Clear old data before next cycle starts.
  for (int v_i = 0; v_i < 8; v_i++) {
    v_dtc[v_i] = 0;
    v_dma[v_i] = 0;
  }
}

void v_shift_src() {
  // Shift source array left by one position.
  // First value goes to last.
  uint16_t v_first = v_src[0];

  for (int v_i = 0; v_i < 7; v_i++) {
    v_src[v_i] = v_src[v_i + 1];
  }

  v_src[7] = v_first;
}

bool v_match(uint16_t * v_a, uint16_t * v_b) {
  // Compare 2 arrays.
  // If any value is different, return false.
  // If all values are same, return true.
  for (int v_i = 0; v_i < 8; v_i++) {
    if (v_a[v_i] != v_b[v_i]) {
      return false;
    }
  }

  return true;
}

void v_show_one(const char * v_name, uint16_t * v_data) {
  // Print buffer name and buffer address.
  Serial.print(v_name);
  Serial.print(" addr : 0x");
  Serial.println((uintptr_t)v_data, HEX);

  // Print all values inside the buffer.
  Serial.print(v_name);
  Serial.print(" : ");
  for (int v_i = 0; v_i < 8; v_i++) {
    Serial.print(v_data[v_i]);
    Serial.print(' ');
  }
  Serial.println();
}

void v_show_result() {
  // Print final result of one cycle.
  Serial.println();
  Serial.print("cycle #");
  Serial.println(v_count);

  v_show_one("SRC", v_src);
  v_show_one("DTC", v_dtc);
  v_show_one("DMA", v_dma);

  // Print time taken by DTC and DMA.
  Serial.print("DTC time ms : ");
  Serial.println(millis() - v_start_time);
  Serial.print("DMA time ms : ");
  Serial.println(v_dma_time - v_start_time);

  // Print short difference note.
  Serial.println("DTC waits for timer event");
  Serial.println("DMA starts immediately by software");
}

bool v_start_cycle() {
  // Start one new cycle.
  v_count++;
  v_dma_done = false;
  v_dtc_started = false;
  v_print_done = false;
  v_start_time = millis();
  v_dma_time = 0;
  v_cycle_done_time = 0;

  // Clear old destination buffer data.
  v_clear_data();

  // Load source and destination into DMA.
  if (R_DMAC_Reset(&v_dma_ctrl, v_src, v_dma, 8) != FSP_SUCCESS) {
    return false;
  }

  // Start DMA by software.
  // DMA does not wait for timer.
  if (R_DMAC_SoftwareStart(&v_dma_ctrl, TRANSFER_START_MODE_REPEAT) != FSP_SUCCESS) {
    return false;
  }

  return true;
}

void setup() {
  // Prepare LED pin.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Start serial monitor.
  Serial.begin(115200);
  delay(1200);
  Serial.println("dtc dma compare");

  // Find free timer.
  v_timer_index = FspTimer::get_available_timer(v_timer_type);
  if (v_timer_index < 0) {
    v_timer_index = FspTimer::get_available_timer(v_timer_type, true);
  }

  if (v_timer_index < 0) {
    Serial.println("timer setup failed");
    return;
  }

  // Start timer in periodic mode at 1 Hz.
  // 1 Hz means 1 event every 1 second.
  if (!v_timer.begin(TIMER_MODE_PERIODIC, v_timer_type, v_timer_index, 1.0f, 50.0f)) {
    Serial.println("timer setup failed");
    return;
  }

  // Enable timer overflow event.
  if (!v_timer.setup_overflow_irq(12)) {
    Serial.println("timer setup failed");
    return;
  }

  if (!v_timer.open()) {
    Serial.println("timer setup failed");
    return;
  }

  if (!v_timer.start()) {
    Serial.println("timer setup failed");
    return;
  }

  // DTC transfer settings.
  // Both source and destination move ahead.
  // Data size is 2 bytes because array type is uint16_t.
  // Block mode means one timer event can copy all 8 values.
  v_dtc_info.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  v_dtc_info.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE;
  v_dtc_info.transfer_settings_word_b.irq = TRANSFER_IRQ_END;
  v_dtc_info.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED;
  v_dtc_info.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  v_dtc_info.transfer_settings_word_b.size = TRANSFER_SIZE_2_BYTE;
  v_dtc_info.transfer_settings_word_b.mode = TRANSFER_MODE_BLOCK;
  v_dtc_info.p_src = v_src;
  v_dtc_info.p_dest = v_dtc;
  v_dtc_info.num_blocks = 1;
  v_dtc_info.length = 8;

  // Connect DTC to timer event.
  // So DTC waits for timer overflow.
  v_dtc_extend.activation_source = v_timer.get_cfg()->cycle_end_irq;
  v_dtc_cfg.p_info = &v_dtc_info;
  v_dtc_cfg.p_extend = &v_dtc_extend;

  // Open and enable DTC.
  if (R_DTC_Open(&v_dtc_ctrl, &v_dtc_cfg) != FSP_SUCCESS) {
    Serial.println("dtc setup failed");
    return;
  }

  if (R_DTC_Enable(&v_dtc_ctrl) != FSP_SUCCESS) {
    Serial.println("dtc setup failed");
    return;
  }

  // DMA transfer settings.
  // DMA will copy same source data to v_dma buffer.
  v_dma_info.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  v_dma_info.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE;
  v_dma_info.transfer_settings_word_b.irq = TRANSFER_IRQ_END;
  v_dma_info.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED;
  v_dma_info.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  v_dma_info.transfer_settings_word_b.size = TRANSFER_SIZE_2_BYTE;
  v_dma_info.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;
  v_dma_info.p_src = v_src;
  v_dma_info.p_dest = v_dma;
  v_dma_info.num_blocks = 0;
  v_dma_info.length = 8;

  // DMA extra settings.
  // Channel 0 is used here.
  // ELC_EVENT_NONE means DMA is started by software.
  v_dma_extend.channel = 0;
  v_dma_extend.irq = FSP_INVALID_VECTOR;
  v_dma_extend.ipl = BSP_IRQ_DISABLED;
  v_dma_extend.offset = 0;
  v_dma_extend.src_buffer_size = 0;
  v_dma_extend.activation_source = ELC_EVENT_NONE;
  v_dma_extend.p_callback = nullptr;
  v_dma_extend.p_context = nullptr;

  v_dma_cfg.p_info = &v_dma_info;
  v_dma_cfg.p_extend = &v_dma_extend;

  // Open and enable DMA.
  if (R_DMAC_Open(&v_dma_ctrl, &v_dma_cfg) != FSP_SUCCESS) {
    Serial.println("dma setup failed");
    return;
  }

  if (R_DMAC_Enable(&v_dma_ctrl) != FSP_SUCCESS) {
    Serial.println("dma setup failed");
    return;
  }

  // Start first cycle.
  if (!v_start_cycle()) {
    Serial.println("dma start failed");
  }
}

void loop() {
  // Stop everything after last cycle.
  if (v_demo_done) {
    return;
  }

  // After one cycle is printed, wait 2 seconds.
  // Then shift source buffer and start next cycle.
  if (v_print_done) {
    if (millis() - v_cycle_done_time >= 2000) {
      // Stop when first value becomes 80.
      if (v_src[0] == 80) {
        v_demo_done = true;
        return;
      }

      v_shift_src();

      if (!v_start_cycle()) {
        Serial.println("dma start failed");
        v_demo_done = true;
      }
    }

    return;
  }

  // First wait for DMA buffer to match source buffer.
  if (!v_dma_done) {
    if (v_match(v_src, v_dma)) {
      v_dma_done = true;
      v_dma_time = millis();

      // After DMA is done, arm DTC.
      // DTC will wait for next timer event.
      if (R_DTC_Reset(&v_dtc_ctrl, v_src, v_dtc, 1) == FSP_SUCCESS) {
        v_dtc_started = true;
      }
    }

    return;
  }

  // After DTC transfer is prepared and ready to start, but it has not started yet, wait for DTC buffer to match source buffer.
  if (v_dtc_started && !v_print_done) {
    if (v_match(v_src, v_dtc)) {
      v_print_done = true;
      digitalWrite(LED_BUILTIN, HIGH);
      v_show_result();
      digitalWrite(LED_BUILTIN, LOW);
      v_cycle_done_time = millis();
    }
  }
}
