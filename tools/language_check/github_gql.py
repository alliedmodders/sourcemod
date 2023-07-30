# Copyright (c) 2023 Peace-Maker
from gql import gql, Client
from gql.transport.aiohttp import AIOHTTPTransport


class GithubGQL:

    def __init__(self, token):
        transport = AIOHTTPTransport(
            url="https://api.github.com/graphql",
            headers={"Authorization": f"Bearer {token}"})
        self.client = Client(transport=transport)

    def get_project(self, orga, project_number):
        query = gql("""
        query getProjectId($login: String!, $projectNumber: Int!){
            organization(login: $login) {
                projectV2(number: $projectNumber) {
                    id
                    fields(first: 100) {
                        nodes {
                            ... on ProjectV2Field {
                                id
                                name
                            }
                            ... on ProjectV2SingleSelectField {
                                id
                                name
                                options {
                                    id
                                    name
                                }
                            }
                            ... on ProjectV2SingleSelectField {
                                id
                                name
                                options {
                                    id
                                    name
                                }
                            }
                        }
                    }
                    items(first: 100) {
                        nodes {
                            id
                            fieldValues(first: 100) {
                                nodes {
                                    ... on ProjectV2ItemFieldTextValue {
                                        text
                                        field {
                                            ... on ProjectV2FieldCommon {
                                                name
                                            }
                                        }
                                    }
                                    ... on ProjectV2ItemFieldSingleSelectValue {
                                        name
                                        field {
                                            ... on ProjectV2FieldCommon {
                                                name
                                            }
                                        }
                                    }
                                }
                            }
                            content {              
                                ... on DraftIssue {
                                    id
                                    title
                                }
                            }
                        }
                    }
                }
            }
        }
        """)
        variables = {"login": orga, "projectNumber": project_number}
        result = self.client.execute(query, variable_values=variables)
        # TODO: Handle pagination
        return result["organization"]["projectV2"]

    def add_draft_issue(self, project_id, title, body):
        query = gql("""
        mutation addDraftIssue($projectId: ID!, $title: String!, $body: String!){
            addProjectV2DraftIssue(
                input: {
                    projectId: $projectId,
                    title: $title,
                    body: $body
                }
            ) {
                projectItem {
                    id
                    content {              
                        ... on DraftIssue {
                            id
                        }
                    }
                }
            }
        }
        """)
        variables = {"projectId": project_id, "title": title, "body": body}
        result = self.client.execute(query, variable_values=variables)
        return result["addProjectV2DraftIssue"]["projectItem"]

    def update_draft_issue(self, issue_id, title, body):
        query = gql("""
        mutation updateDraftIssue($issueId: ID!, $title: String!, $body: String!){
            updateProjectV2DraftIssue(
                input: {
                    draftIssueId: $issueId,
                    title: $title,
                    body: $body
                }
            ) {
                draftIssue {
                    id
                }
            }
        }
        """)
        variables = {"issueId": issue_id, "title": title, "body": body}
        result = self.client.execute(query, variable_values=variables)
        return result["updateProjectV2DraftIssue"]["draftIssue"]["id"]

    def update_item_field_value_option(self, project_id, item_id, field_id,
                                       option_id):
        query = gql("""
        mutation updateDraftIssueStatus($projectId: ID!, $itemId: ID!, $fieldId: ID!, $optionId: String!){
            updateProjectV2ItemFieldValue(
                input: {
                    projectId: $projectId,
                    itemId: $itemId,
                    fieldId: $fieldId,
                    value: {
                        singleSelectOptionId: $optionId
                    }
                }
            ) {
                projectV2Item {
                    id
                }
            }
        }
        """)
        variables = {
            "projectId": project_id,
            "itemId": item_id,
            "fieldId": field_id,
            "optionId": option_id
        }
        result = self.client.execute(query, variable_values=variables)
        return result["updateProjectV2ItemFieldValue"]["projectV2Item"]["id"]
