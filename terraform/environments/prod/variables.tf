# Production Environment Variables

variable "aws_region" {
  type    = string
  default = "us-west-2"
}

variable "project_name" {
  type    = string
  default = "quinedb"
}

variable "environment" {
  type    = string
  default = "prod"
}

variable "vpc_cidr" {
  type    = string
  default = "10.0.0.0/16"
}

variable "kubernetes_version" {
  type    = string
  default = "1.29"
}

variable "node_instance_types" {
  type    = list(string)
  default = ["t3.medium"]
}

variable "capacity_type" {
  type    = string
  default = "ON_DEMAND"
}

variable "node_disk_size" {
  type    = number
  default = 50
}

variable "node_desired_size" {
  type    = number
  default = 2
}

variable "node_min_size" {
  type    = number
  default = 2
}

variable "node_max_size" {
  type    = number
  default = 5
}

variable "image_retention_count" {
  type    = number
  default = 20
}

variable "quinedb_namespace" {
  type    = string
  default = "quinedb"
}

variable "quinedb_image_tag" {
  type    = string
  default = "latest"
}

variable "quinedb_replicas" {
  type    = number
  default = 3
}

variable "quinedb_service_type" {
  type    = string
  default = "ClusterIP"
}

variable "quinedb_log_level" {
  type    = string
  default = "info"
}

variable "quinedb_cpu_request" {
  type    = string
  default = "250m"
}

variable "quinedb_cpu_limit" {
  type    = string
  default = "2000m"
}

variable "quinedb_memory_request" {
  type    = string
  default = "512Mi"
}

variable "quinedb_memory_limit" {
  type    = string
  default = "2Gi"
}

variable "storage_class" {
  type    = string
  default = "gp3"
}

variable "quinedb_storage_size" {
  type    = string
  default = "20Gi"
}
