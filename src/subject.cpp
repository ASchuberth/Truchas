#include "subject.hpp"
#include "pch.hpp"

namespace TRUCHAS_APP_NAMESPACE {

void Subject::addRender(Observer *observer) { observers.push_back(observer); }

void Subject::removeRender(Observer *observer) {}

void Subject::notify() {

  for (const auto &observer : observers) {
    std::cout << "Subject notifies." << std::endl;
    observer->onNotify(int{0}, RenderData{});
  }
}

} // namespace TRUCHAS_APP_NAMESPACE