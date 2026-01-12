# Production Environment - QuineDB on AWS EKS

terraform {
  required_version = ">= 1.5.0"

  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.0"
    }
    kubernetes = {
      source  = "hashicorp/kubernetes"
      version = "~> 2.24"
    }
    tls = {
      source  = "hashicorp/tls"
      version = "~> 4.0"
    }
  }

  # Production uses remote state - uncomment and configure
  # backend "s3" {
  #   bucket         = "quinedb-terraform-state"
  #   key            = "prod/terraform.tfstate"
  #   region         = "us-west-2"
  #   encrypt        = true
  #   dynamodb_table = "terraform-locks"
  # }
}

provider "aws" {
  region = var.aws_region

  default_tags {
    tags = local.tags
  }
}

provider "kubernetes" {
  host                   = module.eks.cluster_endpoint
  cluster_ca_certificate = base64decode(module.eks.cluster_ca_certificate)

  exec {
    api_version = "client.authentication.k8s.io/v1beta1"
    command     = "aws"
    args        = ["eks", "get-token", "--cluster-name", module.eks.cluster_name]
  }
}

locals {
  cluster_name = "${var.project_name}-${var.environment}"

  tags = {
    Project     = var.project_name
    Environment = var.environment
    ManagedBy   = "terraform"
  }
}

data "aws_availability_zones" "available" {
  state = "available"
}

# VPC Module
module "vpc" {
  source = "../../modules/vpc"

  project_name       = var.project_name
  cluster_name       = local.cluster_name
  vpc_cidr           = var.vpc_cidr
  availability_zones = slice(data.aws_availability_zones.available.names, 0, 3)
  enable_nat_gateway = true
  tags               = local.tags
}

# EKS Module
module "eks" {
  source = "../../modules/eks"

  cluster_name        = local.cluster_name
  kubernetes_version  = var.kubernetes_version
  vpc_id              = module.vpc.vpc_id
  public_subnet_ids   = module.vpc.public_subnet_ids
  private_subnet_ids  = module.vpc.private_subnet_ids
  node_instance_types = var.node_instance_types
  capacity_type       = var.capacity_type
  node_disk_size      = var.node_disk_size
  node_desired_size   = var.node_desired_size
  node_min_size       = var.node_min_size
  node_max_size       = var.node_max_size
  enable_logging      = true
  tags                = local.tags
}

# ECR Module
module "ecr" {
  source = "../../modules/ecr"

  repository_name       = var.project_name
  image_retention_count = var.image_retention_count
  image_tag_mutability  = "IMMUTABLE"
  scan_on_push          = true
  tags                  = local.tags
}

# QuineDB Application Module
module "quinedb" {
  source = "../../modules/quinedb"

  namespace         = var.quinedb_namespace
  image             = "${module.ecr.repository_url}:${var.quinedb_image_tag}"
  image_pull_policy = "Always"
  replicas          = var.quinedb_replicas
  quinedb_port      = 6379
  service_type      = var.quinedb_service_type
  log_level         = var.quinedb_log_level
  cpu_request       = var.quinedb_cpu_request
  cpu_limit         = var.quinedb_cpu_limit
  memory_request    = var.quinedb_memory_request
  memory_limit      = var.quinedb_memory_limit
  storage_class     = var.storage_class
  storage_size      = var.quinedb_storage_size

  depends_on = [module.eks]
}
