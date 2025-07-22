#include "model.hpp"
#include "pch.hpp"

Model::Model() : mId{0} {}

Model::Model(int id) : mId{id} {}

Model::~Model() {}

void Model::addPoint(glm::vec3 p) {}

void Model::notify() {

  for (const auto &observer : observers) {
    std::cout << "Model notifies." << std::endl;
    observer->onNotify(mId, mRenderables);
  }
}

void Model::clearRender() {
  mRenderables = TRUCHAS_APP_NAMESPACE::RenderData{};
}
