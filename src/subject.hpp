#pragma once

#include "observer.hpp"

namespace TRUCHAS_APP_NAMESPACE {

class Subject {

public:
  void addRender(Observer *observer);
  void removeRender(Observer *observer);
  virtual void notify();

protected:
  std::vector<Observer *> observers;

private:
};
} // namespace TRUCHAS_APP_NAMESPACE