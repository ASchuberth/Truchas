#pragma once
#include "subject.hpp"

class Edge {};

class Model : public TRUCHAS_APP_NAMESPACE::Subject {

public:
  Model();
  Model(int id);
  ~Model();

  void addPoint(glm::vec3 p);
  void notify() override;

  void clearRender();

private:
  int mId;
  TRUCHAS_APP_NAMESPACE::RenderData mRenderables;
};