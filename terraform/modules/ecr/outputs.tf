# ECR Module Outputs

output "repository_url" {
  description = "URL of the ECR repository"
  value       = aws_ecr_repository.quinedb.repository_url
}

output "repository_arn" {
  description = "ARN of the ECR repository"
  value       = aws_ecr_repository.quinedb.arn
}

output "repository_name" {
  description = "Name of the ECR repository"
  value       = aws_ecr_repository.quinedb.name
}

output "ecr_push_policy_arn" {
  description = "ARN of the IAM policy for pushing images"
  value       = aws_iam_policy.ecr_push.arn
}
