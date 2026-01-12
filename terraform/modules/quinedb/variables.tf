# QuineDB Module Variables

variable "namespace" {
  description = "Kubernetes namespace for QuineDB"
  type        = string
  default     = "quinedb"
}

variable "image" {
  description = "Docker image for QuineDB"
  type        = string
}

variable "image_pull_policy" {
  description = "Image pull policy"
  type        = string
  default     = "IfNotPresent"
}

variable "replicas" {
  description = "Number of QuineDB replicas"
  type        = number
  default     = 1
}

variable "quinedb_port" {
  description = "Port for QuineDB RESP protocol"
  type        = number
  default     = 6379
}

variable "service_type" {
  description = "Kubernetes service type"
  type        = string
  default     = "ClusterIP"
}

variable "log_level" {
  description = "Log level for QuineDB"
  type        = string
  default     = "info"
}

variable "cpu_request" {
  description = "CPU request for QuineDB container"
  type        = string
  default     = "100m"
}

variable "cpu_limit" {
  description = "CPU limit for QuineDB container"
  type        = string
  default     = "1000m"
}

variable "memory_request" {
  description = "Memory request for QuineDB container"
  type        = string
  default     = "256Mi"
}

variable "memory_limit" {
  description = "Memory limit for QuineDB container"
  type        = string
  default     = "1Gi"
}

variable "storage_class" {
  description = "Storage class for persistent volume"
  type        = string
  default     = "gp2"
}

variable "storage_size" {
  description = "Size of persistent storage"
  type        = string
  default     = "5Gi"
}
