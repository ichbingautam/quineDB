# QuineDB Terraform Infrastructure

Infrastructure as Code for deploying QuineDB to AWS EKS.

## Prerequisites

- [Terraform](https://terraform.io) >= 1.5.0
- [AWS CLI](https://aws.amazon.com/cli/) configured with credentials
- [kubectl](https://kubernetes.io/docs/tasks/tools/)

## Architecture

```
terraform/
├── environments/
│   ├── dev/          # Development environment
│   └── prod/         # Production environment
└── modules/
    ├── vpc/          # VPC, subnets, NAT gateway
    ├── eks/          # EKS cluster and node groups
    ├── ecr/          # Container registry
    └── quinedb/      # Kubernetes resources
```

## Quick Start

### 1. Initialize Terraform

```bash
cd terraform/environments/dev
terraform init
```

### 2. Review the Plan

```bash
terraform plan
```

### 3. Apply Infrastructure

```bash
terraform apply
```

### 4. Configure kubectl

```bash
# Get the command from Terraform output
$(terraform output -raw configure_kubectl)
```

### 5. Push Docker Image to ECR

```bash
# Login to ECR
$(terraform output -raw docker_login_command)

# Build and push
docker build -t quinedb:latest ../../..
docker tag quinedb:latest $(terraform output -raw ecr_repository_url):latest
docker push $(terraform output -raw ecr_repository_url):latest
```

### 6. Verify Deployment

```bash
kubectl get pods -n quinedb
kubectl get svc -n quinedb
```

## Environment Comparison

| Feature | Development | Production |
|---------|-------------|------------|
| Node Type | t3.small (SPOT) | t3.medium (ON_DEMAND) |
| Node Count | 1 | 2-5 (auto-scaling) |
| Replicas | 1 | 3 |
| Storage | 5Gi (gp2) | 20Gi (gp3) |
| Logging | Disabled | Enabled |
| AZs | 2 | 3 |

## Remote State (Recommended for Production)

Create an S3 bucket and DynamoDB table for state management:

```bash
aws s3 mb s3://quinedb-terraform-state --region us-west-2
aws dynamodb create-table \
    --table-name terraform-locks \
    --attribute-definitions AttributeName=LockID,AttributeType=S \
    --key-schema AttributeName=LockID,KeyType=HASH \
    --billing-mode PAY_PER_REQUEST
```

Then uncomment the `backend "s3"` block in `main.tf`.

## Cost Optimization

Development environment uses:

- **SPOT instances** for up to 90% cost savings
- Single node cluster
- Smaller storage volumes
- Logging disabled

## Troubleshooting

### Cannot connect to EKS cluster

```bash
aws eks update-kubeconfig --region us-west-2 --name quinedb-dev
```

### Pod stuck in Pending

```bash
kubectl describe pod -n quinedb
kubectl get events -n quinedb
```

### Check QuineDB logs

```bash
kubectl logs -n quinedb -l app=quinedb -f
```

## Cleanup

```bash
terraform destroy
```

> **Warning**: This will delete all resources including persistent data.
