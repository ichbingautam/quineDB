# Development Environment Variables

# AWS Configuration
variable "aws_region" {
  description = "AWS region"
  type        = string
  default     = "us-west-2"
}

variable "project_name" {
  description = "Project name"
  type        = string
  default     = "quinedb"
}

variable "environment" {
  description = "Environment name"
  type        = string
  default     = "dev"
}

# VPC Configuration
variable "vpc_cidr" {
  description = "VPC CIDR block"
  type        = string
  default     = "10.0.0.0/16"
}

variable "enable_nat_gateway" {
  description = "Enable NAT Gateway"
  type        = bool
  default     = true
}

# EKS Configuration
variable "kubernetes_version" {
  description = "Kubernetes version"
  type        = string
  default     = "1.29"
}

variable "node_instance_types" {
  description = "EC2 instance types for nodes"
  type        = list(string)
  default     = ["t3.small"]
}

variable "capacity_type" {
  description = "Node capacity type"
  type        = string
  default     = "ON_DEMAND"
}

variable "node_disk_size" {
  description = "Node disk size in GB"
  type        = number
  default     = 20
}

variable "node_desired_size" {
  description = "Desired node count"
  type        = number
  default     = 1
}

variable "node_min_size" {
  description = "Minimum node count"
  type        = number
  default     = 1
}

variable "node_max_size" {
  description = "Maximum node count"
  type        = number
  default     = 2
}

variable "enable_logging" {
  description = "Enable EKS logging"
  type        = bool
  default     = false
}

# ECR Configuration
variable "image_retention_count" {
  description = "Number of images to retain"
  type        = number
  default     = 5
}

# QuineDB Configuration
variable "quinedb_namespace" {
  description = "Kubernetes namespace"
  type        = string
  default     = "quinedb"
}

variable "quinedb_image_tag" {
  description = "Docker image tag"
  type        = string
  default     = "latest"
}

variable "quinedb_replicas" {
  description = "Number of replicas"
  type        = number
  default     = 1
}

variable "quinedb_service_type" {
  description = "Service type"
  type        = string
  default     = "ClusterIP"
}

variable "quinedb_log_level" {
  description = "Log level"
  type        = string
  default     = "debug"
}

variable "quinedb_cpu_request" {
  description = "CPU request"
  type        = string
  default     = "100m"
}

variable "quinedb_cpu_limit" {
  description = "CPU limit"
  type        = string
  default     = "500m"
}

variable "quinedb_memory_request" {
  description = "Memory request"
  type        = string
  default     = "128Mi"
}

variable "quinedb_memory_limit" {
  description = "Memory limit"
  type        = string
  default     = "512Mi"
}

variable "storage_class" {
  description = "Storage class"
  type        = string
  default     = "gp2"
}

variable "quinedb_storage_size" {
  description = "Storage size"
  type        = string
  default     = "5Gi"
}
