#include <vision_msgs_rviz_plugins/detection_3d.hpp>

#include <memory>

namespace rviz_plugins
{

    Detection3DDisplay::Detection3DDisplay()
    {
        only_edge_property_ = new rviz_common::properties::BoolProperty(
            "Only Edge", false, "Display only edges of the boxes", this, SLOT(updateEdge()));
        line_width_property_ = new rviz_common::properties::FloatProperty(
            "Line Width", 0.05, "Line width of edges", this, SLOT(updateLineWidth()));
        alpha_property_ = new rviz_common::properties::FloatProperty(
            "Alpha", 1.0, "Transparency", this, SLOT(updateAlpha()));
        show_score_property_ = new rviz_common::properties::BoolProperty(
            "Show Score", false, "Display score next to bounding boxes", this, SLOT(updateShowScores()));
    }

    Detection3DDisplay::~Detection3DDisplay()
    {
        delete only_edge_property_;
        delete line_width_property_;
        delete alpha_property_;
        delete show_score_property_;
    }

    void Detection3DDisplay::onInitialize()
    {
        RTDClass::onInitialize();
        m_marker_common->initialize(context_, scene_node_);

        topic_property_->setValue("detection3_d");
        topic_property_->setDescription("Detection3D topic to subscribe to.");
        
        line_width_property_->setMax(0.1);
        line_width_property_->setMin(0.01);
        line_width_property_->hide();

        alpha_property_->setMax(1.0);
        alpha_property_->setMin(0.1);

        line_width = 0.2;
        alpha = 1.0;

        only_edge_ = false;
        show_score_ = false;
    }

    void Detection3DDisplay::load(const rviz_common::Config &config)
    {
        Display::load(config);
        m_marker_common->load(config);
    }

    void Detection3DDisplay::processMessage(
        vision_msgs::msg::Detection3D::ConstSharedPtr msg)
    {
        latest_msg = msg;
        if (!only_edge_)
        {
            showBoxes(msg, show_score_);
        }
        else
        {
            showEdges(msg, show_score_);
        }
    }

    void Detection3DDisplay::update(float wall_dt, float ros_dt)
    {
        m_marker_common->update(wall_dt, ros_dt);
    }

    void Detection3DDisplay::reset()
    {
        RosTopicDisplay::reset();
        m_marker_common->clearMarkers();
        edges_.clear();
    }

    void Detection3DDisplay::updateEdge()
    {
        only_edge_ = only_edge_property_->getBool();
        if (only_edge_)
        {
            line_width_property_->show();
        }
        else
        {
            line_width_property_->hide();
        }
        // Imediately apply attribute
        if (latest_msg)
        {
            if (only_edge_)
            {
                showEdges(latest_msg, show_score_);
            }
            else
            {
                showBoxes(latest_msg, show_score_);
            }
        }
    }

    void Detection3DDisplay::updateLineWidth()
    {
        line_width = line_width_property_->getFloat();
        if (latest_msg)
        {
            processMessage(latest_msg);
        }
    }

    void Detection3DDisplay::updateAlpha()
    {
        alpha = alpha_property_->getFloat();
        if (latest_msg)
        {
            processMessage(latest_msg);
        }
    }

    void Detection3DDisplay::updateShowScores()
    {
        show_score_ = show_score_property_->getBool();
        if (latest_msg)
        {
            processMessage(latest_msg);
        }
    }

} // namespace rviz_plugins

// Export the plugin
#include <pluginlib/class_list_macros.hpp> // NOLINT
PLUGINLIB_EXPORT_CLASS(rviz_plugins::Detection3DDisplay, rviz_common::Display)