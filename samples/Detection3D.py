#!/usr/bin/env python3

from math import pi
import random 

import rclpy
from rclpy.node import Node
import tf_transformations
from std_msgs.msg import Header
from vision_msgs.msg import Detection3D
from vision_msgs.msg import BoundingBox3D
from vision_msgs.msg import ObjectHypothesisWithPose


class pub_detection3_d_array(Node):
    def __init__(self):
        super().__init__("pub_detection3_d_sample")
        self.__pub = self.create_publisher(
            Detection3D, "detection3_d", 10)
        self.__timer = self.create_timer(0.1, self.pub_sample)
        self.__counter = 0
        self.__header = Header()
        self.__msg_def = {
            "score": [0.0, 1.0, 2.0, 3.0, 1.0],
            "obj_id": ["", "", "car", "cyclist", "tree"]
        }

    def create_msg(self, bbox: BoundingBox3D, scores, obj_ids) -> Detection3D:
        msg = Detection3D()
        msg.header = self.__header
        msg.bbox = bbox
        for score, obj_id in zip(scores, obj_ids):
            obj = ObjectHypothesisWithPose()
            obj.hypothesis.score = score
            obj.hypothesis.class_id = obj_id
            msg.results.append(obj)
        return msg

    def pub_sample(self):
        while self.__pub.get_subscription_count() == 0:
            return
        self.__header.stamp = self.get_clock().now().to_msg()
        self.__header.frame_id = "map"
        
        # Reset counter and indices if counter is a multiple of 30
        if self.__counter % 10 == 0:
            self.__counter = 0
        
        # Get the current score and object ID
        score = random.sample(self.__msg_def["score"], 1)[0]
        obj_id = random.sample(self.__msg_def["obj_id"], 1)[0]
        
        # Create a single Detection3D message
        bbox = BoundingBox3D()
        quat = tf_transformations.quaternion_about_axis(
            (self.__counter % 100) * pi * 2 / 100.0, [0, 0, 1])
        bbox.center.orientation.x = quat[0]
        bbox.center.orientation.y = quat[1]
        bbox.center.orientation.z = quat[2]
        bbox.center.orientation.w = quat[3]
        # Set the center position to a fixed value
        bbox.center.position.x = 1.5
        bbox.center.position.y = 0.0
        bbox.size.x = (self.__counter % 10 + 1) * 0.1
        bbox.size.y = ((self.__counter + 1) % 5 + 1) * 0.1
        bbox.size.z = ((self.__counter + 2) % 10 + 1) * 0.1
        detection_msg = self.create_msg(
            bbox=bbox, scores=[score], obj_ids=[obj_id]
        )
        
        # Publish the Detection3D message
        self.__pub.publish(detection_msg)
        self.__counter += 1
        

def main(args=None):
    rclpy.init(args=args)
    node = pub_detection3_d_array()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
