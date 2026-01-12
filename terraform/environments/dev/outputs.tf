# Development Environment Outputs

# VPC
output "vpc_id" {
  description = "VPC ID"
  value       = module.vpc.vpc_id
}

# EKS
output "cluster_name" {
  description = "EKS cluster name"
  value       = module.eks.cluster_name
}

output "cluster_endpoint" {
  description = "EKS cluster endpoint"
  value       = module.eks.cluster_endpoint
}

output "cluster_ca_certificate" {
  description = "EKS CA certificate (base64)"
  value       = module.eks.cluster_ca_certificate
  sensitive   = true
}

# ECR
output "ecr_repository_url" {
  description = "ECR repository URL"
  value       = module.ecr.repository_url
}

# QuineDB
output "quinedb_namespace" {
  description = "QuineDB namespace"
  value       = module.quinedb.namespace
}

output "quinedb_connection_string" {
  description = "QuineDB internal connection string"
  value       = module.quinedb.connection_string
}

# Useful Commands
output "configure_kubectl" {
  description = "Command to configure kubectl"
  value       = "aws eks update-kubeconfig --region ${var.aws_region} --name ${module.eks.cluster_name}"
}

output "docker_login_command" {
  description = "Command to login to ECR"
  value       = "aws ecr get-login-password --region ${var.aws_region} | docker login --username AWS --password-stdin ${module.ecr.repository_url}"
}
