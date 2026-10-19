#pragma once
#define ESPHOME_LOG_TAG(name, tag) static constexpr const char *const name = tag
#define ESP_LOGV(...) do {} while (0)
#define ESP_LOGD(...) do {} while (0)
#define ESP_LOGW(...) do {} while (0)
