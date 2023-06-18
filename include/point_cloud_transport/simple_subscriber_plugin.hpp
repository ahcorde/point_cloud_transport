// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: Czech Technical University in Prague .. 2019, paplhjak .. 2009, Willow Garage, Inc.

/*
 *
 * BSD 3-Clause License
 *
 * Copyright (c) Czech Technical University in Prague
 * Copyright (c) 2019, paplhjak
 * Copyright (c) 2009, Willow Garage, Inc.
 *
 *        All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 *        modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 *       THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *       AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *       IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *       DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *       FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *       DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *       CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *       OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *       OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include "rclcpp/subscription.hpp"

#include <point_cloud_transport/subscriber_plugin.hpp>

namespace point_cloud_transport
{

/**
 * Base class to simplify implementing most plugins to Subscriber.
 *
 * The base class simplifies implementing a SubscriberPlugin in the common case that
 * all communication with the matching PublisherPlugin happens over a single ROS
 * topic using a transport-specific message type. SimpleSubscriberPlugin is templated
 * on the transport-specific message type.
 *
 * A subclass need implement only two methods:
 * - getTransportName() from SubscriberPlugin
 * - internalCallback() - processes a message and invoked the user PointCloud2 callback if
 * appropriate.
 *
 * For access to the parameter server and name remappings, use nh().
 *
 * getTopicToSubscribe() controls the name of the internal communication topic. It
 * defaults to \<base topic\>/\<transport name\>.
 */
template<class M>
class SimpleSubscriberPlugin : public SubscriberPlugin
{
public:
  virtual ~SimpleSubscriberPlugin()
  {
  }

  std::string getTopic() const override
  {
    if (impl_)
    {
      return impl_->sub_->get_topic_name();
    }
    return {};
  }

  uint32_t getNumPublishers() const override
  {
    if (impl_)
    {
      return impl_->sub_->get_publisher_count();
    }
    return 0;
  }

  void shutdown() override
  {
    impl_.reset();
  }

protected:
  /**
   * \brief Process a message. Must be implemented by the subclass.
   *
   * @param message A message from the PublisherPlugin.
   * @param user_cb The user PointCloud2 callback to invoke, if appropriate.
   */

  virtual void internalCallback(
    const typename std::shared_ptr<const M> & message,
    const Callback & user_cb) = 0;

  /**
   * \brief Return the communication topic name for a given base topic.
   *
   * Defaults to \<base topic\>/\<transport name\>.
   */
  virtual std::string getTopicToSubscribe(const std::string & base_topic) const
  {
    return base_topic + "/" + getTransportName();
  }

  void subscribeImpl(
    rclcpp::Node * node,
    const std::string & base_topic,
    const Callback & callback,
    rmw_qos_profile_t custom_qos) override
  {
    impl_ = std::make_unique<Impl>();
    // Push each group of transport-specific parameters into a separate sub-namespace
    // ros::NodeHandle param_nh(transport_hints.getParameterNH(), getTransportName());
    //
    auto qos = rclcpp::QoS(rclcpp::QoSInitialization::from_rmw(custom_qos), custom_qos);
    impl_->sub_ = node->create_subscription<M>(
      getTopicToSubscribe(base_topic), qos,
      [this, callback](const typename std::shared_ptr<const M> msg) {
        internalCallback(msg, callback);
      });
  }

  void subscribeImplWithOptions(
    rclcpp::Node * node,
    const std::string & base_topic,
    const Callback & callback,
    rmw_qos_profile_t custom_qos,
    rclcpp::SubscriptionOptions options)
  {
    impl_ = std::make_unique<Impl>();
    // Push each group of transport-specific parameters into a separate sub-namespace
    // ros::NodeHandle param_nh(transport_hints.getParameterNH(), getTransportName());
    //
    auto qos = rclcpp::QoS(rclcpp::QoSInitialization::from_rmw(custom_qos), custom_qos);
    impl_->sub_ = node->create_subscription<M>(
      getTopicToSubscribe(base_topic), qos,
      [this, callback](const typename std::shared_ptr<const M> msg) {
        internalCallback(msg, callback);
      },
      options);
  }

private:
  struct Impl
  {
    rclcpp::SubscriptionBase::SharedPtr sub_;
  };

  std::unique_ptr<Impl> impl_;
};

}
