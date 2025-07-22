#pragma once

namespace TRUCHAS_APP_NAMESPACE {

class RenderData {

  friend std::ostream &operator<<(std::ostream &os, const RenderData &ri) {

    os << "RenderItems:" << std::endl;

    if (ri.points.empty())
      return os;
    os << "Points:" << std::endl;
    for (const auto &p : ri.points) {

      os << std::format("P,{:.2f},{:.2f},{:.2f}", p.x, p.y, p.z);
      os << std::endl;
    }
    os << std::endl;
    return os;
  }

public:
  std::vector<glm::vec3> points;
  std::vector<glm::vec3> gridLine;
};

class Observer {

public:
  Observer();
  virtual ~Observer();

  virtual void onNotify(int id, const RenderData &renderables);

private:
};
} // namespace TRUCHAS_APP_NAMESPACE