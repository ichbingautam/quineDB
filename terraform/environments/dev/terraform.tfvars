# Development Environment - terraform.tfvars
# Customize these values for your development environment

aws_region   = "us-west-2"
project_name = "quinedb"
environment  = "dev"

# Cost-optimized for development
node_instance_types = ["t3.small"]
node_desired_size   = 1
node_min_size       = 1
node_max_size       = 2
capacity_type       = "SPOT" # Use spot instances to save costs

# QuineDB Configuration
quinedb_replicas     = 1
quinedb_storage_size = "5Gi"
quinedb_log_level    = "debug"
