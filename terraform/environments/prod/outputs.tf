# Production Environment Outputs

output "vpc_id" {
  value = module.vpc.vpc_id
}

output "cluster_name" {
  value = module.eks.cluster_name
}

output "cluster_endpoint" {
  value = module.eks.cluster_endpoint
}

output "cluster_ca_certificate" {
  value     = module.eks.cluster_ca_certificate
  sensitive = true
}

output "ecr_repository_url" {
  value = module.ecr.repository_url
}

output "quinedb_namespace" {
  value = module.quinedb.namespace
}

output "quinedb_connection_string" {
  value = module.quinedb.connection_string
}

output "configure_kubectl" {
  value = "aws eks update-kubeconfig --region ${var.aws_region} --name ${module.eks.cluster_name}"
}

output "docker_login_command" {
  value = "aws ecr get-login-password --region ${var.aws_region} | docker login --username AWS --password-stdin ${module.ecr.repository_url}"
}
