# Production Environment - terraform.tfvars

aws_region   = "us-west-2"
project_name = "quinedb"
environment  = "prod"

# Production-grade instances
node_instance_types = ["t3.medium", "t3.large"]
node_desired_size   = 2
node_min_size       = 2
node_max_size       = 5
capacity_type       = "ON_DEMAND"
node_disk_size      = 50

# QuineDB High Availability
quinedb_replicas     = 3
quinedb_storage_size = "20Gi"
quinedb_log_level    = "info"
quinedb_cpu_request  = "250m"
quinedb_cpu_limit    = "2000m"
quinedb_memory_request = "512Mi"
quinedb_memory_limit   = "2Gi"
storage_class          = "gp3"
