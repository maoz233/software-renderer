/**
 * @file layer.h
 * @author Mao Zhang (mao.zhang233@gmail.com)
 * @brief
 * @version 1.0
 * @date 2023-04-12
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef SOFTWARE_RENDERER_INCLUDE_LAYER_H_
#define SOFTWARE_RENDERER_INCLUDE_LAYER_H_

namespace swr {

class Layer {
 public:
  virtual ~Layer() = default;

  virtual void OnUIRender() = 0;
};

}  // namespace swr

#endif  // SOFTWARE_RENDERER_INCLUDE_LAYER_H_
