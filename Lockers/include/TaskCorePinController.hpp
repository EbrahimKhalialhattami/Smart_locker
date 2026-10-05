#ifndef TASK_CORE_PIN_CONTROLLER_HPP
#define TASK_CORE_PIN_CONTROLLER_HPP
#include <Arduino.h>
#include <cstdint>

class TaskCorePinController final {
public:
TaskCorePinController() = default;
TaskCorePinController& pinTaskToCpuCore(void (*taskHandler)(void*),std::uint16_t coreId = 1U) {
 _coreId=coreId;
 
 if (_task != nullptr) 
 {

    vTaskDelete(_task);
    _task = nullptr;

 }
 
    xTaskCreatePinnedToCore(taskHandler,"task",_taskStackSize,nullptr,_taskpriorityLevel,&_task,_coreId);


    return *this;
};

// priority level
TaskCorePinController& setTaskPriorityLevel(std::uint16_t priorityLevel) {
    _taskpriorityLevel = priorityLevel;
    return *this;

}
// stack size
TaskCorePinController& setTaskStackSize(std::uint16_t stackSize) {
    _taskStackSize = stackSize;
    return *this;
}

// core id
TaskCorePinController& setTaskCoreId(std::uint16_t coreId) {
    _coreId = coreId;
    return *this;}

void suspendTask() {
    if (_task != nullptr) {
        vTaskSuspend(_task);
    }
}

void resumeTask() {
    if (_task != nullptr) {
        vTaskResume(_task);
    }
}


 private :
 TaskHandle_t _task{nullptr}; // default task handle
 std::uint16_t _coreId{1}; // default core id
 std::uint16_t _taskpriorityLevel{1}; // default priority level
 std::uint16_t _taskStackSize{10*1000}; // 10 KB stack size

};



#endif
