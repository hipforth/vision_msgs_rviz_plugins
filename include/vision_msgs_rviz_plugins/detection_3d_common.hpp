#ifndef DETECTION_3D_COMMON_DISPLAY_HPP_
#define DETECTION_3D_COMMON_DISPLAY_HPP_

#include <boost/algorithm/string.hpp>
#include <memory>
#include <string>

#include <rviz_common/display.hpp>
#include <rviz_common/properties/bool_property.hpp>
#include <rviz_common/properties/float_property.hpp>
#include <rviz_default_plugins/displays/marker/marker_common.hpp>
#include <rviz_default_plugins/displays/marker_array/marker_array_display.hpp>
#include <rviz_rendering/objects/billboard_line.hpp>

#include <vision_msgs/msg/bounding_box3_d.hpp>
#include <vision_msgs/msg/detection3_d.hpp>
#include <vision_msgs/msg/detection3_d_array.hpp>

#include <QWidget>

#include "vision_msgs_rviz_plugins/visibility_control.hpp"

typedef std::shared_ptr<rviz_rendering::BillboardLine> BillboardLinePtr;

namespace rviz_plugins
{
    template <class MessageType>
    class Detection3DCommon : public rviz_common::RosTopicDisplay<MessageType>
    {
    public:
        using MarkerCommon = rviz_default_plugins::displays::MarkerCommon;
        using Marker = visualization_msgs::msg::Marker;
        using BoundingBox3D = vision_msgs::msg::BoundingBox3D;
        using Detection3DArray = vision_msgs::msg::Detection3DArray;
        
        Detection3DCommon()
            : rviz_common::RosTopicDisplay<MessageType>()
            , line_width(0.2), alpha()
            , m_marker_common(std::make_unique<MarkerCommon>(this)){};
        ~Detection3DCommon()
        {
        };

    protected:
        float line_width, alpha;
        std::unique_ptr<MarkerCommon> m_marker_common;
        std::vector<BillboardLinePtr> edges_;

        visualization_msgs::msg::Marker::SharedPtr get_marker(
            const vision_msgs::msg::BoundingBox3D &box) const
        {
            auto marker = std::make_shared<Marker>();

            marker->type = Marker::CUBE;
            marker->action = Marker::ADD;

            marker->pose.position.x = static_cast<double>(box.center.position.x);
            marker->pose.position.y = static_cast<double>(box.center.position.y);
            marker->pose.position.z = static_cast<double>(box.center.position.z);
            marker->pose.orientation.x = static_cast<double>(box.center.orientation.x);
            marker->pose.orientation.y = static_cast<double>(box.center.orientation.y);
            marker->pose.orientation.z = static_cast<double>(box.center.orientation.z);
            marker->pose.orientation.w = static_cast<double>(box.center.orientation.w);
            marker->scale.x = static_cast<double>(box.size.x);
            marker->scale.y = static_cast<double>(box.size.y);
            marker->scale.z = static_cast<double>(box.size.z);

            return marker;
        }
        QColor getColor(bool is_tracking, std::string id = "")
        {
            QColor color;
            if (id == "")
            {
                if (is_tracking)
                    color.setRgb(22, 255, 80, 255);
                else
                    color.setRgb(255, 22, 80, 255);
            }
            else
            {
                if (boost::to_lower_copy(id) == "car")
                    color.setRgb(255, 165, 0); // orange
                else if (boost::to_lower_copy(id) == "person")
                    color.setRgb(0, 0, 255); // blue
                else if (boost::to_lower_copy(id) == "cyclist")
                    color.setRgb(255, 255, 0); // yellow
                else if (boost::to_lower_copy(id) == "motorcycle")
                    color.setRgb(230, 230, 250); // purple
                else
                    color.setRgb(190, 190, 190); // gray
            }
            return color;
        }

        void showBoxes(vision_msgs::msg::Detection3DArray::ConstSharedPtr &msg)
        {
            edges_.clear();
            for (size_t idx = 0U; idx < msg->detections.size(); idx++)
            {
                const auto marker_ptr = get_marker(msg->detections[idx].bbox);
                QColor color = getColor(msg->detections[idx].is_tracking);
                if (msg->detections[idx].results.size() > 0)
                {
                    auto iter = std::max_element(msg->detections[idx].results.begin(),
                                                msg->detections[idx].results.end(),
                                                [](const auto & a, const auto & b){
                                                    return a.score < b.score;
                                                });
                    color = getColor(msg->detections[idx].is_tracking, iter->id);                    
                }
                marker_ptr->color.r = color.red() / 255.0;
                marker_ptr->color.g = color.green() / 255.0;
                marker_ptr->color.b = color.blue() / 255.0;
                marker_ptr->color.a = alpha;
                marker_ptr->ns = "bounding_box";
                marker_ptr->header = msg->header;
                marker_ptr->id = idx;
                m_marker_common->addMessage(marker_ptr);
            }
        }

        void allocateBillboardLines(size_t num)
        {
            if (num > edges_.size())
            {
                for (size_t i = edges_.size(); i < num; i++)
                {
                    BillboardLinePtr line(new rviz_rendering::BillboardLine(
                        this->context_->getSceneManager(), this->scene_node_));
                    edges_.push_back(line);
                }
            }
            else if (num < edges_.size())
            {
                edges_.resize(num);
            }
        }

        void showEdges(vision_msgs::msg::Detection3DArray::ConstSharedPtr &msg)
        {
            m_marker_common->clearMarkers();

            allocateBillboardLines(msg->detections.size());

            for (size_t idx = 0; idx < msg->detections.size(); idx++)
            {
                vision_msgs::msg::BoundingBox3D box = msg->detections[idx].bbox;
                bool is_tracking = msg->detections[idx].is_tracking;
                QColor color = getColor(is_tracking);
                if (msg->detections[idx].results.size() > 0)
                {
                    auto iter = std::max_element(msg->detections[idx].results.begin(),
                                                msg->detections[idx].results.end(),
                                                [](const auto & a, const auto & b){
                                                    return a.score < b.score;
                                                });
                    color = getColor(msg->detections[idx].is_tracking, iter->id);                    
                }
                geometry_msgs::msg::Vector3 dimensions = box.size;

                BillboardLinePtr edge = edges_[idx];
                edge->clear();
                Ogre::Vector3 position;
                Ogre::Quaternion quaternion;
                if (!this->context_->getFrameManager()->transform(msg->header, box.center,
                                                                  position,
                                                                  quaternion))
                {
                    std::ostringstream oss;
                    oss << "Error transforming pose";
                    oss << " from frame '" << msg->header.frame_id << "'";
                    oss << " to frame '" << qPrintable(this->fixed_frame_) << "'";
                    RVIZ_COMMON_LOG_ERROR_STREAM(oss.str());
                    this->setStatus(rviz_common::properties::StatusProperty::Error, "Transform", QString::fromStdString(oss.str()));
                    return;
                }
                edge->setPosition(position);
                edge->setOrientation(quaternion);

                edge->setMaxPointsPerLine(2);
                edge->setNumLines(12);
                edge->setLineWidth(line_width);
                edge->setColor(color.red() / 255.0,
                               color.green() / 255.0,
                               color.blue() / 255.0,
                               alpha);

                Ogre::Vector3 A, B, C, D, E, F, G, H;
                A[0] = dimensions.x / 2.0;
                A[1] = dimensions.y / 2.0;
                A[2] = dimensions.z / 2.0;
                B[0] = -dimensions.x / 2.0;
                B[1] = dimensions.y / 2.0;
                B[2] = dimensions.z / 2.0;
                C[0] = -dimensions.x / 2.0;
                C[1] = -dimensions.y / 2.0;
                C[2] = dimensions.z / 2.0;
                D[0] = dimensions.x / 2.0;
                D[1] = -dimensions.y / 2.0;
                D[2] = dimensions.z / 2.0;

                E[0] = dimensions.x / 2.0;
                E[1] = dimensions.y / 2.0;
                E[2] = -dimensions.z / 2.0;
                F[0] = -dimensions.x / 2.0;
                F[1] = dimensions.y / 2.0;
                F[2] = -dimensions.z / 2.0;
                G[0] = -dimensions.x / 2.0;
                G[1] = -dimensions.y / 2.0;
                G[2] = -dimensions.z / 2.0;
                H[0] = dimensions.x / 2.0;
                H[1] = -dimensions.y / 2.0;
                H[2] = -dimensions.z / 2.0;

                edge->addPoint(A);
                edge->addPoint(B);
                edge->finishLine();
                edge->addPoint(B);
                edge->addPoint(C);
                edge->finishLine();
                edge->addPoint(C);
                edge->addPoint(D);
                edge->finishLine();
                edge->addPoint(D);
                edge->addPoint(A);
                edge->finishLine();
                edge->addPoint(E);
                edge->addPoint(F);
                edge->finishLine();
                edge->addPoint(F);
                edge->addPoint(G);
                edge->finishLine();
                edge->addPoint(G);
                edge->addPoint(H);
                edge->finishLine();
                edge->addPoint(H);
                edge->addPoint(E);
                edge->finishLine();
                edge->addPoint(A);
                edge->addPoint(E);
                edge->finishLine();
                edge->addPoint(B);
                edge->addPoint(F);
                edge->finishLine();
                edge->addPoint(C);
                edge->addPoint(G);
                edge->finishLine();
                edge->addPoint(D);
                edge->addPoint(H);
            }
        }
    };
}

#endif